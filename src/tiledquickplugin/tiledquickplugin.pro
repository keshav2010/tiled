TEMPLATE = lib
TARGET = tiledquickplugin
macx {
    DESTDIR = "../../bin/Tiled Quick.app/Contents/qml/org/mapeditor/Tiled"
} else {
    DESTDIR = ../../qml/org/mapeditor/Tiled
}
QT += qml quick
CONFIG += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)
uri = org.mapeditor.Tiled

# Input
SOURCES += \
    tiledquickplugin_plugin.cpp \
    tiledquickplugin.cpp

HEADERS += \
    tiledquickplugin_plugin.h \
    tiledquickplugin.h

OTHER_FILES = qmldir

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/$$DESTDIR/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

qmldir.files = qmldir
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}
