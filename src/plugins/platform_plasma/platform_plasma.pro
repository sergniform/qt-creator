TEMPLATE = lib
TARGET = PlatformPlasma

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

LIBS += -ldl
CONFIG += link_pkgconfig
PKGCONFIG += glib-2.0

HEADERS += platform_plasmaplugin.h
SOURCES += platform_plasmaplugin.cpp

OTHER_FILES += PlatformPlasma.pluginspec.in
