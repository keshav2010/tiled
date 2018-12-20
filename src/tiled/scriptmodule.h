/*
 * scriptmodule.h
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

#include "documentmanager.h"

#include <QObject>

namespace Tiled {

class LoggingInterface;

class EditableAsset;

/**
 * Initial point of access to Tiled functionality from JavaScript.
 */
class ScriptModule : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QString platform READ platform)
    Q_PROPERTY(QString arch READ arch)

    Q_PROPERTY(Tiled::EditableAsset *activeAsset READ activeAsset WRITE setActiveAsset NOTIFY activeAssetChanged)
    Q_PROPERTY(QList<QObject*> openAssets READ openAssets)

public:
    ScriptModule(QObject *parent = nullptr);
    ~ScriptModule() override;

    QString version() const;
    QString platform() const;
    QString arch() const;

    EditableAsset *activeAsset() const;
    bool setActiveAsset(EditableAsset *asset) const;

    QList<QObject*> openAssets() const;

signals:
    void assetCreated(Tiled::EditableAsset *asset);
    void assetOpened(Tiled::EditableAsset *asset);
    void assetAboutToBeSaved(Tiled::EditableAsset *asset);
    void assetSaved(Tiled::EditableAsset *asset);
    void assetAboutToBeClosed(Tiled::EditableAsset *asset);

    void activeAssetChanged(Tiled::EditableAsset *asset);

public slots:
    void trigger(const QByteArray &actionName) const;

    void alert(const QString &text, const QString &title = QString()) const;
    bool confirm(const QString &text, const QString &title = QString()) const;
    QString prompt(const QString &label, const QString &text = QString(), const QString &title = QString()) const;

    void log(const QString &text) const;
    void error(const QString &text) const;

private slots:
    void documentCreated(Document *document);
    void documentOpened(Document *document);
    void documentAboutToBeSaved(Document *document);
    void documentSaved(Document *document);
    void documentAboutToClose(Document *document);
    void currentDocumentChanged(Document *document);

private:
    LoggingInterface *mLogger;
};

} // namespace Tiled
