TEMPLATE = lib
TARGET = PlatformUnity

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

LIBS += -ldl
CONFIG += link_pkgconfig
PKGCONFIG += glib-2.0
QT += dbus

HEADERS += platform_unityplugin.h
SOURCES += platform_unityplugin.cpp

OTHER_FILES += PlatformUnity.pluginspec.in
