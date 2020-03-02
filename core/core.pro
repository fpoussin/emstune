#-------------------------------------------------
#
# Project created by QtCreator 2012-05-03T11:03:53
#
#-------------------------------------------------

QT += core gui opengl printsupport widgets quick qml

TARGET = emstudio
TEMPLATE = app
INCLUDEPATH += src
OBJECTS_DIR = obj
MOC_DIR = obj
UI_DIR = obj
CONFIG += console

INCLUDEPATH += $$PWD/../lib/core
DEPENDPATH += $$PWD/../lib/core

include(QsLog/QsLog.pri)

win32 {
  message("Building for win32")
        DEFINES += GIT_COMMIT=$$system(\"c:/program files/git/bin/git.exe\" describe --dirty=-DEV --always)
        DEFINES += GIT_HASH=$$system(\"c:/program files/git/bin/git.exe\" log -n 1 --pretty=format:%H)
} else:mac {
  QMAKE_CXXFLAGS += -Werror
  INCLUDEPATH += /opt/local/include
  LIBS += -L/opt/local/lib
  DEFINES += GIT_COMMIT=$$system(git describe --dirty=-DEV --always)
  DEFINES += GIT_HASH=$$system(git log -n 1 --pretty=format:%H)
  DEFINES += GIT_DATE=\""$$system(date)"\"
} else:unix {
  message("Building for *nix")
  isEmpty($$PREFIX) {
    PREFIX = /usr/local
    message("EMSTune using default install prefix " $$PREFIX)
  } else {
    message("EMSTune using custom install prefix " $$PREFIX)
  }
  DEFINES += INSTALL_PREFIX=$$PREFIX
  target.path = $$PREFIX/bin
  dashboard.path = $$PREFIX/share/emstudio/dashboards
  dashboard.files += src/gauges.qml
  warninglabel.path = $$PREFIX/share/emstudio/dashboards/WarningLabel
  warninglabel.files += src/WarningLabel/WarningLabel.qml
  wizards.path = $$PREFIX/share/emstudio/wizards
  wizards.files += wizards/BenchTest.qml
  wizards.files += wizards/DecoderOffset.qml
  wizards.files += wizards/wizard.qml
  config.path = $$PREFIX/share/emstudio/definitions
  config.files += libreems.config.json
  INSTALLS += target config dashboard wizards warninglabel
  LIBS += -lGL -lGLU -lglut
        DEFINES += GIT_COMMIT=$$system(git describe --dirty=-DEV --always)
        DEFINES += GIT_HASH=$$system(git log -n 1 --pretty=format:%H)
        DEFINES += GIT_DATE=\""$$system(date)"\"
}

SOURCES += src/main.cpp\
  src/mainwindow.cpp \
  src/logloader.cpp \
  src/gaugewidget.cpp \
  src/gaugeitem.cpp \
  src/comsettings.cpp \
  src/rawdatadisplay.cpp \
  src/qhexedit.cpp \
  src/qhexedit_p.cpp \
  src/xbytearray.cpp \
  src/commands.cpp \
  src/rawdataview.cpp \
  src/gaugeview.cpp \
  src/emsinfoview.cpp \
  src/flagview.cpp \
  src/tableview.cpp \
  src/tableview2d.cpp \
  src/packetstatusview.cpp \
  src/aboutview.cpp \
  #src/memorylocation.cpp \
  src/tableview3d.cpp \
  src/interrogateprogressview.cpp \
  src/readonlyramview.cpp \
  src/overviewprogressitemdelegate.cpp \
  src/dataview.cpp \
  src/emsstatus.cpp \
  src/tablemap3d.cpp \
  src/tablewidget.cpp \
  src/configview.cpp \
  src/tablewidgetdelegate.cpp \
  src/parameterview.cpp \
  src/parameterwidget.cpp \
  src/wizardview.cpp \
  src/firmwaremetadata.cpp \
  src/abstractgaugeitem.cpp \
  src/bargaugeitem.cpp \
  src/gaugeutil.cpp \
  src/roundgaugeitem.cpp \
  src/scalarparam.cpp \
  src/comboparam.cpp \
  src/ramdiffwindow.cpp \
  src/qcustomplot.cpp \
  src/tableviewnew3d.cpp \
  src/pluginmanager.cpp \
    src/firmwaredebugview.cpp


HEADERS  += src/mainwindow.h \
  src/logloader.h \
  src/gaugewidget.h \
  src/gaugeitem.h \
  src/comsettings.h \
  src/rawdatadisplay.h \
  src/qhexedit.h \
  src/qhexedit_p.h \
  src/xbytearray.h \
  src/commands.h \
  src/rawdataview.h \
  src/gaugeview.h \
  src/emsinfoview.h \
  src/flagview.h \
  src/tableview.h \
  src/tableview2d.h \
  src/packetstatusview.h \
  src/aboutview.h \
  src/tableview3d.h \
  src/interrogateprogressview.h \
  ../lib/core/tabledata.h \
  src/readonlyramview.h \
  src/overviewprogressitemdelegate.h \
  src/dataview.h \
  src/emsstatus.h \
  src/tablemap3d.h \
  src/tablewidget.h \
  src/configview.h \
  ../lib/core/table2ddata.h \
  ../lib/core/table3ddata.h \
  ../lib/core/memorymetadata.h \
  src/tablewidgetdelegate.h \
  ../lib/core/emscomms.h \
  ../lib/core/datapacketdecoder.h \
  src/parameterview.h \
  src/parameterwidget.h \
  src/wizardview.h \
  src/firmwaremetadata.h \
  src/abstractgaugeitem.h \
  src/bargaugeitem.h \
  src/gaugeutil.h \
  src/roundgaugeitem.h \
  src/gaugeutil.h \
  src/scalarparam.h \
  src/comboparam.h \
  src/ramdiffwindow.h \
  src/qcustomplot.h \
  src/tableviewnew3d.h \
  src/qcustomplot.h \
  src/pluginmanager.h \
    src/firmwaredebugview.h

FORMS    += src/mainwindow.ui \
  src/comsettings.ui \
  src/emsinfo.ui \
  src/datagauges.ui \
  src/datatables.ui \
  src/rawdatadisplay.ui \
  src/dataflags.ui \
  src/rawdataview.ui \
  src/tableview2d.ui \
  src/packetstatusview.ui \
  src/aboutview.ui \
  src/tableview3d.ui \
  src/interrogateprogressview.ui \
  src/readonlyramview.ui \
  src/emsstatus.ui \
  src/configview.ui \
  src/parameterview.ui \
  src/firmwaremetadata.ui \
  src/scalarparam.ui \
  src/comboparam.ui \
  src/parameterwidget.ui \
  src/ramdiffwindow.ui \
  src/pluginmanager.ui \
    src/firmwaredebugview.ui
SUBDIRS += plugins
OTHER_FILES += \
  README.md \
  wizards/BenchTest.qml \
  wizards/DecoderOffset.qml \
  wizards/wizard.qml \
  wizards/EngineConfig.qml \
  wizards/ActualDecoderOffset.qml \
  src/WarningLabel/WarningLabel.qml
