/*
 * editableobjectgroup.h
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

#pragma once

#include "editablelayer.h"
#include "editablemapobject.h"
#include "objectgroup.h"

namespace Tiled {

class EditableObjectGroup : public EditableLayer
{
    Q_OBJECT

    Q_PROPERTY(int objectCount READ objectCount)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    EditableObjectGroup(EditableMap *map,
                        ObjectGroup *objectGroup,
                        QObject *parent = nullptr);

    int objectCount() const;
    Q_INVOKABLE Tiled::EditableMapObject *objectAt(int index);
    Q_INVOKABLE void removeObjectAt(int index);
    Q_INVOKABLE void removeObject(Tiled::EditableMapObject *editableMapObject);
    Q_INVOKABLE void insertObjectAt(int index, Tiled::EditableMapObject *editableMapObject);
    Q_INVOKABLE void addObject(Tiled::EditableMapObject *editableMapObject);
    QColor color() const;

public slots:
    void setColor(const QColor &color);

private:
    ObjectGroup *objectGroup() const;
};


inline int EditableObjectGroup::objectCount() const
{
    return objectGroup()->objectCount();
}

inline QColor EditableObjectGroup::color() const
{
    return objectGroup()->color();
}

inline ObjectGroup *EditableObjectGroup::objectGroup() const
{
    return static_cast<ObjectGroup*>(layer());
}

} // namespace Tiled
