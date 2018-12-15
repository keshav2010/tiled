/*
 * createtileobjecttool.cpp
 * Copyright 2014, Martin Ziel <martin.ziel.com>
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

#include "createtileobjecttool.h"

#include "mapdocument.h"
#include "mapobject.h"
#include "mapobjectitem.h"
#include "maprenderer.h"
#include "snaphelper.h"
#include "tile.h"
#include "utils.h"

using namespace Tiled;
using namespace Tiled::Internal;

CreateTileObjectTool::CreateTileObjectTool(QObject *parent)
    : CreateObjectTool(parent)
{
    QIcon icon(QLatin1String(":images/24x24/insert-image.png"));
    icon.addFile(QLatin1String(":images/48x48/insert-image.png"));
    setIcon(icon);
    Utils::setThemeIcon(this, "insert-image");
<<<<<<< HEAD
    mToolInfo = tr("First select the tile to place in the Tilesets view. Hold <b>Left mouse button</b> on the map to move the object and release it to place object. Holding <b>Ctrl</b> snaps the object to grid.");
    languageChanged();
=======
    languageChangedImpl();
>>>>>>> 4a3496224dea749224cad47a5d0de3c7800a060d
}

void CreateTileObjectTool::mouseMovedWhileCreatingObject(const QPointF &pos, Qt::KeyboardModifiers modifiers)
{
    const MapRenderer *renderer = mapDocument()->renderer();

    const QSize imgSize = mNewMapObjectItem->mapObject()->cell().tile()->size();
    const QPointF diff(-imgSize.width() / 2, imgSize.height() / 2);
    QPointF pixelCoords = renderer->screenToPixelCoords(pos + diff);

    SnapHelper(renderer, modifiers).snap(pixelCoords);

    mNewMapObjectItem->mapObject()->setPosition(pixelCoords);
    mNewMapObjectItem->syncWithMapObject();
}

void CreateTileObjectTool::languageChanged()
{
    CreateObjectTool::languageChanged();
    languageChangedImpl();
}

void CreateTileObjectTool::languageChangedImpl()
{
    setName(tr("Insert Tile"));
    setShortcut(QKeySequence(tr("T")));
}

MapObject *CreateTileObjectTool::createNewMapObject()
{
    if (!tile())
        return nullptr;

    MapObject *newMapObject = new MapObject;
    newMapObject->setShape(MapObject::Rectangle);
    newMapObject->setCell(Cell(tile()));
    newMapObject->setSize(tile()->size());
    return newMapObject;
}
