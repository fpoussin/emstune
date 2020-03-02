TEMPLATE = lib
TARGET = core

win32:QMAKE_LFLAGS += -shared

include(core.pri)
