/*
 * editablemap.cpp
 * Copyright 2018, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "editablemap.h"

#include "addremovelayer.h"
#include "addremovemapobject.h"
#include "changelayer.h"
#include "changemapproperty.h"
#include "changeselectedarea.h"
#include "editablelayer.h"
#include "editablemapobject.h"
#include "editableobjectgroup.h"
#include "editabletilelayer.h"
#include "grouplayer.h"
#include "movemapobject.h"
#include "resizemap.h"
#include "resizetilelayer.h"
#include "scriptmanager.h"

#include <imagelayer.h>
#include <mapobject.h>
#include <maprenderer.h>
#include <objectgroup.h>
#include <tilelayer.h>

#include <QUndoStack>

#include "qtcompat_p.h"

namespace Tiled {

EditableMap::EditableMap(MapDocument *mapDocument, QObject *parent)
    : EditableAsset(parent)
    , mMapDocument(mapDocument)
    , mSelectedArea(mapDocument)
{
    connect(map(), &Map::sizeChanged, this, &EditableMap::sizeChanged);
    connect(map(), &Map::tileWidthChanged, this, &EditableMap::tileWidthChanged);
    connect(map(), &Map::tileHeightChanged, this, &EditableMap::tileHeightChanged);

    connect(mapDocument, &Document::fileNameChanged, this, &EditableAsset::fileNameChanged);
    connect(mapDocument, &MapDocument::layerRemoved, this, &EditableMap::detachEditableLayer);
    connect(mapDocument, &MapDocument::objectsRemoved, this, &EditableMap::detachMapObjects);
}

EditableMap::~EditableMap()
{
    // Operate on copy since original container will get modified
    const auto editableLayers = mEditableLayers;
    for (auto editable : editableLayers)
        editable->detach();

    const auto editableMapObjects = mEditableMapObjects;
    for (auto editable : editableMapObjects)
        editable->detach();
}

QString EditableMap::fileName() const
{
    return mapDocument()->fileName();
}

EditableLayer *EditableMap::layerAt(int index)
{
    if (index < 0 || index >= layerCount()) {
        ScriptManager::instance().throwError(tr("Index out of range"));
        return nullptr;
    }

    Layer *layer = map()->layerAt(index);
    return editableLayer(layer);
}

void EditableMap::removeLayerAt(int index)
{
    if (index < 0 || index >= layerCount()) {
        ScriptManager::instance().throwError(tr("Index out of range"));
        return;
    }

    push(new RemoveLayer(mapDocument(), index, nullptr));
}

void EditableMap::removeLayer(EditableLayer *editableLayer)
{
    if (!editableLayer) {
        ScriptManager::instance().throwError(tr("Invalid argument"));
        return;
    }

    int index = map()->layers().indexOf(editableLayer->layer());
    if (index == -1) {
        ScriptManager::instance().throwError(tr("Layer not found"));
        return;
    }

    removeLayerAt(index);
}

void EditableMap::insertLayerAt(int index, EditableLayer *editableLayer)
{
    if (index < 0 || index > layerCount()) {
        ScriptManager::instance().throwError(tr("Index out of range"));
        return;
    }

    if (!editableLayer) {
        ScriptManager::instance().throwError(tr("Invalid argument"));
        return;
    }

    if (editableLayer->map()) {
        ScriptManager::instance().throwError(tr("Layer already part of a map"));
        return;
    }

    push(new AddLayer(mapDocument(), index, editableLayer->layer(), nullptr));
    editableLayer->attach(this);
}

void EditableMap::addLayer(EditableLayer *editableLayer)
{
    insertLayerAt(layerCount(), editableLayer);
}

void EditableMap::setTileWidth(int value)
{
    push(new ChangeMapProperty(mapDocument(), ChangeMapProperty::TileWidth, value));
}

void EditableMap::setTileHeight(int value)
{
    push(new ChangeMapProperty(mapDocument(), ChangeMapProperty::TileHeight, value));
}

void EditableMap::setInfinite(bool value)
{
    push(new ChangeMapProperty(mapDocument(), ChangeMapProperty::Infinite, value));
}

void EditableMap::setHexSideLength(int value)
{
    push(new ChangeMapProperty(mapDocument(), ChangeMapProperty::HexSideLength, value));
}

void EditableMap::setStaggerAxis(Map::StaggerAxis value)
{
    push(new ChangeMapProperty(mapDocument(), value));
}

void EditableMap::setStaggerIndex(Map::StaggerIndex value)
{
    push(new ChangeMapProperty(mapDocument(), value));
}

void EditableMap::setOrientation(Map::Orientation value)
{
    push(new ChangeMapProperty(mapDocument(), value));
}

void EditableMap::setRenderOrder(Map::RenderOrder value)
{
    push(new ChangeMapProperty(mapDocument(), value));
}

void EditableMap::setBackgroundColor(const QColor &value)
{
    push(new ChangeMapProperty(mapDocument(), value));
}

void EditableMap::setLayerDataFormat(Map::LayerDataFormat value)
{
    push(new ChangeMapProperty(mapDocument(), value));
}

/**
 * Custom intersects check necessary because QRectF::intersects wants a
 * non-empty area of overlap, but we should also consider overlap with empty
 * area as intersection.
 *
 * Results for rectangles with negative size are undefined.
 */
static bool intersects(const QRectF &a, const QRectF &b)
{
    return a.right() >= b.left() &&
            a.bottom() >= b.top() &&
            a.left() <= b.right() &&
            a.top() <= b.bottom();
}

static bool visibleIn(const QRectF &area, MapObject *object,
                      const MapRenderer &renderer)
{
    QRectF boundingRect = renderer.boundingRect(object);

    if (object->rotation() != 0) {
        // Rotate around object position
        QPointF pos = renderer.pixelToScreenCoords(object->position());
        boundingRect.translate(-pos);

        QTransform transform;
        transform.rotate(object->rotation());
        boundingRect = transform.mapRect(boundingRect);

        boundingRect.translate(pos);
    }

    return intersects(area, boundingRect);
}

/**
 * Resize this map to the given \a size, while at the same time shifting
 * the contents by \a offset. If \a removeObjects is true then all objects
 * which are outside the map will be removed.
 */
void EditableMap::resize(const QSize &size,
                         const QPoint &offset,
                         bool removeObjects)
{
    if (size.isEmpty())
        return;

    const QRegion movedSelection = mapDocument()->selectedArea().translated(offset);
    const QRect newArea = QRect(-offset, size);
    const QRectF visibleArea = renderer()->boundingRect(newArea);

    const QPointF origin = renderer()->tileToPixelCoords(QPointF());
    const QPointF newOrigin = renderer()->tileToPixelCoords(-offset);
    const QPointF pixelOffset = origin - newOrigin;

    // Resize the map and each layer
    QUndoCommand *command = new QUndoCommand(tr("Resize Map"));

    QList<MapObject *> objectsToRemove;

    LayerIterator iterator(map());
    while (Layer *layer = iterator.next()) {
        switch (layer->layerType()) {
        case Layer::TileLayerType: {
            TileLayer *tileLayer = static_cast<TileLayer*>(layer);
            new ResizeTileLayer(mapDocument(), tileLayer, size, offset, command);
            break;
        }
        case Layer::ObjectGroupType: {
            ObjectGroup *objectGroup = static_cast<ObjectGroup*>(layer);

            for (MapObject *o : objectGroup->objects()) {
                if (removeObjects && !visibleIn(visibleArea, o, *renderer())) {
                    // Remove objects that will fall outside of the map
                    objectsToRemove.append(o);
                } else {
                    QPointF oldPos = o->position();
                    QPointF newPos = oldPos + pixelOffset;
                    new MoveMapObject(mapDocument(), o, newPos, oldPos, command);
                }
            }
            break;
        }
        case Layer::ImageLayerType: {
            // Adjust image layer by changing its offset
            auto imageLayer = static_cast<ImageLayer*>(layer);
            new SetLayerOffset(mapDocument(), layer,
                               imageLayer->offset() + pixelOffset,
                               command);
            break;
        }
        case Layer::GroupLayerType: {
            // Recursion handled by LayerIterator
            break;
        }
        }
    }

    if (!objectsToRemove.isEmpty())
        new RemoveMapObjects(mapDocument(), objectsToRemove, command);

    new ResizeMap(mapDocument(), size, command);
    new ChangeSelectedArea(mapDocument(), movedSelection, command);

    mapDocument()->undoStack()->push(command);

    // TODO: Handle layers that don't match the map size correctly
}

void EditableMap::detachEditableLayer(Layer *layer)
{
    auto iterator = mEditableLayers.find(layer);
    if (iterator != mEditableLayers.end())
        (*iterator)->detach();

    if (GroupLayer *groupLayer = layer->asGroupLayer()) {
        for (Layer *childLayer : groupLayer->layers())
            detachEditableLayer(childLayer);
    } else if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
        detachMapObjects(objectGroup->objects());
    }
}

void EditableMap::detachMapObjects(const QList<MapObject *> &mapObjects)
{
    for (MapObject *mapObject : mapObjects) {
        auto iterator = mEditableMapObjects.find(mapObject);
        if (iterator != mEditableMapObjects.end())
            (*iterator)->detach();
    }
}

EditableLayer *EditableMap::editableLayer(Layer *layer)
{
    Q_ASSERT(layer->map() == map());

    EditableLayer* &editableLayer = mEditableLayers[layer];
    if (!editableLayer) {
        switch (layer->layerType()) {
        case Layer::TileLayerType:
            editableLayer = new EditableTileLayer(this, static_cast<TileLayer*>(layer));
            break;
        case Layer::ObjectGroupType:
            editableLayer = new EditableObjectGroup(this, static_cast<ObjectGroup*>(layer));
            break;
        default:
            editableLayer = new EditableLayer(this, layer);
            break;
        }
    }

    return editableLayer;
}

EditableMapObject *EditableMap::editableMapObject(MapObject *mapObject)
{
    Q_ASSERT(mapObject->objectGroup());
    Q_ASSERT(mapObject->objectGroup()->map() == map());

    EditableMapObject* &editableMapObject = mEditableMapObjects[mapObject];
    if (!editableMapObject)
        editableMapObject = new EditableMapObject(this, mapObject);

    return editableMapObject;
}

} // namespace Tiled
