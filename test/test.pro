#-------------------------------------------------
#
# Project created by QtCreator 2015-04-28T18:57:34
#
#-------------------------------------------------

QT       += core serialport

QT       -= gui

TARGET = test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../lib/core
#INCLUDEPATH += ../emstune/plugins/libreems/
include (../core/QsLog/QsLog.pri)
SOURCES += main.cpp \
    ../plugins/libreems/datafield.cpp \
    ../plugins/libreems/emsdata.cpp \
    ../plugins/libreems/feconfigdata.cpp \
    ../plugins/libreems/fedatapacketdecoder.cpp \
    ../plugins/libreems/fememorymetadata.cpp \
    ../plugins/libreems/ferawdata.cpp \
    ../plugins/libreems/fetable2ddata.cpp \
    ../plugins/libreems/fetable3ddata.cpp \
    ../plugins/libreems/freeemscomms.cpp \
    ../plugins/libreems/memorylocation.cpp \
    serialport.cpp \
    serialrxthread.cpp \
    protocolencoder.cpp \
    protocoldecoder.cpp \
    packetdecoder.cpp \
    testcontrol.cpp

HEADERS += \
    ../plugins/libreems/datafield.h \
    ../plugins/libreems/emsdata.h \
    ../plugins/libreems/feconfigdata.h \
    ../plugins/libreems/fedatapacketdecoder.h \
    ../plugins/libreems/fememorymetadata.h \
    ../plugins/libreems/ferawdata.h \
    ../plugins/libreems/fetable2ddata.h \
    ../plugins/libreems/fetable3ddata.h \
    ../plugins/libreems/freeemscomms.h \
    ../plugins/libreems/memorylocation.h \
    ../plugins/libreems/packet.h \
    ../plugins/libreems/serialport.h \
    ../plugins/libreems/serialrxthread.h \
    ../plugins/libreems/packetdecoder.h \
    ../plugins/libreems/protocoldecoder.h \
    ../plugins/libreems/protocolencoder.h \
    ../lib/core/configdata.h \
    ../lib/core/datapacketdecoder.h \
    ../lib/core/rawdata.h \
    ../lib/core/table3ddata.h \
    ../lib/core/table2ddata.h \
    ../lib/core/emscomms.h \
    ../plugins/libreems/protocolencoder.h \
    testcontrol.h
