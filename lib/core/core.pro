TEMPLATE = lib
TARGET = core

CONFIG += c++11

win32:QMAKE_LFLAGS += -shared

include(core.pri)
