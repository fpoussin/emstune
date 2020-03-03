/************************************************************************************
 * EMStudio - Open Source ECU tuning software                                       *
 * Copyright (C) 2013  Michael Carpenter (malcom2073@gmail.com)                     *
 *                                                                                  *
 * This file is a part of EMStudio                                                  *
 *                                                                                  *
 * EMStudio is free software; you can redistribute it and/or                        *
 * modify it under the terms of the GNU Lesser General Public                       *
 * License as published by the Free Software Foundation, version                    *
 * 2.1 of the License.                                                              *
 *                                                                                  *
 * EMStudio is distributed in the hope that it will be useful,                      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 * Lesser General Public License for more details.                                  *
 *                                                                                  *
 * You should have received a copy of the GNU Lesser General Public                 *
 * License along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA   *
 ************************************************************************************/

#include "mainwindow.h"
#include <QFileDialog>
//#include "datafield.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSettings>
#include <tableview2d.h>

#include "QsLog.h"
#include "logloader.h"
#include "tableviewnew3d.h"
#include "wizardview.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>

#define define2string_p(x) #x
#define define2string(x) define2string_p(x)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    qRegisterMetaType<MemoryLocationInfo>("MemoryLocationInfo");
    qRegisterMetaType<DataType>("DataType");
    qRegisterMetaType<SerialPortStatus>("SerialPortStatus");

    QLOG_INFO() << "EMStudio commit:" << define2string(GIT_COMMIT);
    QLOG_INFO() << "Full hash:" << define2string(GIT_HASH);
    QLOG_INFO() << "Build date:" << define2string(GIT_DATE);

    const QProcessEnvironment env(QProcessEnvironment::systemEnvironment());

    m_interrogationInProgress = false;
    m_offlineMode = false;
    m_checkEmsDataInUse = false;
    m_currentEcuClock = -1;
    m_EcuResetPopup = false;
    m_interrogateProgressMdiWindow = 0;

    m_progressView = 0;
    m_emsComms = 0;
    m_ramDiffWindow = 0;
    m_interrogationInProgress = false;
    m_debugLogs = false;
    m_emsSilenceTimer = new QTimer(this);

    connect(m_emsSilenceTimer, SIGNAL(timeout()), this, SLOT(emsCommsSilenceTimerTick()));
    m_emsSilenceTimer->start(50);

    //	emsData = new EmsData();
    //	connect(emsData,SIGNAL(updateRequired(unsigned short)),this,SLOT(updateDataWindows(unsigned short)));

    //Create this, even though we only need it during offline to online transitions.
    //	checkEmsData = new EmsData();

    m_settingsFile = "settings.ini";
    //TODO: Figure out proper directory names

#ifdef Q_OS_WIN
    QString appDataDir(env.value("AppData"));
    if (appDataDir.isEmpty()) {
        appDataDir = env.value("USERPROFILE");
    }

    appDataDir = appDataDir.replace("\\", "/");
    if (!QDir(appDataDir).exists("EMStudio")) {
        QDir(appDataDir).mkpath("EMStudio");
    }
    m_defaultsDir = QApplication::instance()->applicationDirPath();

    m_settingsDir = appDataDir + "/" + "EMStudio";
    //%HOMEPATH%//m_localHomeDir
    m_localHomeDir = env.value("USERPROFILE").replace("\\", "/");
    m_localHomeDir += "/EMStudio";
    //m_settingsFile = appDataDir + "/" + "EMStudio/EMStudio-config.ini";
//#elif Q_OS_MAC //<- Does not exist. Need OSX checking capabilities somewhere...
//Linux and Mac function identically here for now...
#else //if Q_OS_LINUX
    QString appDataDir(env.value("HOME"));

    if (!QDir(appDataDir).exists(".EMStudio")) {
        QDir(appDataDir).mkpath(".EMStudio");
    }

    m_defaultsDir = QString(define2string(INSTALL_PREFIX)) + "/share/emstudio";
    m_settingsDir = appDataDir + "/" + ".EMStudio";
    m_localHomeDir = appDataDir + "/" + "EMStudio";
    //m_settingsFile = appDataDir + "/" + ".EMStudio/EMStudio-config.ini";
#endif
    if (!QFile::exists(m_localHomeDir + "/logs")) {
        QDir(m_localHomeDir).mkpath(m_localHomeDir + "/logs");
    }
    //Settings file should ALWAYS be the one in the settings dir. No reason to have it anywhere else.
    m_settingsFile = m_settingsDir + "/EMStudio-config.ini";

    QString decoderfilestr;
    QStringList decoder_file_locations(
            { m_settingsDir + "/definitions/decodersettings.json",
              m_defaultsDir + "/definitions/decodersettings.json",
              "decodersettings.json" });
    for (int i = 0; i < decoder_file_locations.size(); i++) {
        if (QFile::exists(decoder_file_locations.at(i))) {
            decoderfilestr = decoder_file_locations.at(i);
            break;
        }
    }

    if (QFile::exists(decoderfilestr)) {

        QLOG_INFO() << "Loading decoder file from:" << decoderfilestr;

        QFile decoderfile(decoderfilestr);
        decoderfile.open(QIODevice::ReadOnly);
        QByteArray decoderfilebytes(decoderfile.readAll());
        decoderfile.close();

        QJsonDocument document(QJsonDocument::fromJson(decoderfilebytes));
        QJsonObject decodertopmap(document.object());
        QJsonObject decoderlocationmap(decodertopmap.value("locations").toObject());
        QString str(decoderlocationmap.value("locationid").toString());
        bool ok = false;
        unsigned short locid = str.toUInt(&ok, 16);
        QJsonArray decodervalueslist(decoderlocationmap.value("values").toArray());
        QList<ConfigBlock> blocklist;
        for (int i = 0; i < decodervalueslist.size(); i++) {
            QJsonObject tmpmap = decodervalueslist.at(i).toObject();
            ConfigBlock block;
            block.setLocationId(locid);
            block.setName(tmpmap.value("name").toString());
            block.setType(tmpmap.value("type").toString());
            block.setElementSize(tmpmap.value("sizeofelement").toInt());
            block.setSize(tmpmap.value("size").toInt());
            block.setOffset(tmpmap.value("offset").toInt());
            block.setSizeOverride(tmpmap.value("sizeoverride").toString());
            block.setSizeOverrideMult(tmpmap.value("sizeoverridemult").toDouble());
            QList<QPair<QString, double>> calclist;
            QJsonArray calcliststr = tmpmap.value("calc").toArray();
            for (int j = 0; j < calcliststr.size(); j++) {
                //QLOG_TRACE() << "XCalc:" << calcliststr.at(j).toObject().value("type").toString() << calcliststr.at(j)[j].toMap()["value"].toDouble();
                calclist.append(QPair<QString, double>(calcliststr.at(j).toObject().value("type").toString(), calcliststr.at(j).toObject().value("value").toDouble()));
            }
            block.setCalc(calclist);
            blocklist.append(block);
        }
        m_configBlockMap[locid] = blocklist;
    }

    ui.setupUi(this);
    ui.actionDisconnect->setEnabled(false);
    ui.actionInterrogation_Progress->setEnabled(false);
    setWindowTitle(QString("EMStudio ") + QString(define2string(GIT_COMMIT)));
    menuBar()->setNativeMenuBar(false);

    m_currentRamLocationId = 0;
    //    populateDataFields();
    m_waitingForRamWriteConfirmation = false;
    m_waitingForFlashWriteConfirmation = false;

    m_emsinfo.emstudioCommit = define2string(GIT_COMMIT);
    m_emsinfo.emstudioHash = define2string(GIT_HASH);

    connect(ui.actionSave_Offline_Data, SIGNAL(triggered()), this, SLOT(menu_file_saveOfflineDataClicked()));
    connect(ui.actionEMS_Status, SIGNAL(triggered()), this, SLOT(menu_windows_EmsStatusClicked()));
    connect(ui.actionLoad_Offline_Data, SIGNAL(triggered()), this, SLOT(menu_file_loadOfflineDataClicked()));
    connect(ui.actionSettings, SIGNAL(triggered()), this, SLOT(menu_settingsClicked()));
    connect(ui.actionConnect, SIGNAL(triggered()), this, SLOT(menu_connectClicked()));
    connect(ui.actionDisconnect, SIGNAL(triggered()), this, SLOT(menu_disconnectClicked()));
    connect(ui.actionEMS_Info, SIGNAL(triggered()), this, SLOT(menu_windows_EmsInfoClicked()));
    connect(ui.actionGauges, SIGNAL(triggered()), this, SLOT(menu_windows_GaugesClicked()));
    connect(ui.actionTables, SIGNAL(triggered()), this, SLOT(menu_windows_TablesClicked()));
    connect(ui.actionFlags, SIGNAL(triggered()), this, SLOT(menu_windows_FlagsClicked()));
    connect(ui.actionInterrogation_Progress, SIGNAL(triggered()), this, SLOT(menu_windows_interrogateProgressViewClicked()));
    connect(ui.actionExit_3, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui.actionPacket_Status, SIGNAL(triggered()), this, SLOT(menu_windows_PacketStatusClicked()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(menu_aboutClicked()));
    connect(ui.actionEnable_Datalogs, SIGNAL(triggered()), this, SLOT(menu_enableDatalogsClicked()));
    connect(ui.actionDisable_Datalog_Stream, SIGNAL(triggered()), this, SLOT(menu_disableDatalogsClicked()));
    connect(ui.actionParameter_View, SIGNAL(triggered()), this, SLOT(menu_windows_ParameterViewClicked()));
    connect(ui.actionFirmware_Metadata, SIGNAL(triggered()), this, SLOT(menu_windows_firmwareMetadataClicked()));
    connect(ui.actionFirmware_Debug, SIGNAL(triggered()), this, SLOT(menu_windows_firmwareDebugClicked()));

    m_emsInfo = 0;
    m_dataTables = 0;
    m_dataFlags = 0;
    m_dataGauges = 0;
    m_pluginLoader = new QPluginLoader(this);
    QStringList plugin_locations(
            { "plugins/libfreeemsplugin.so",
              "/usr/share/emstudio/plugins/libfreeemsplugin.so",
              "plugins/freeemsplugin.lib" });
    for (int i = 0; i < plugin_locations.size(); i++) {
        if (QFile::exists(plugin_locations.at(i))) {
            m_pluginFileName = plugin_locations.at(i);
            QLOG_INFO() << "Loading plugin from:" << m_pluginFileName;
            break;
        }
    }

    //m_pluginFileName = "plugins/libmsplugin.so";
    //m_pluginFileName = "plugins/libo5eplugin.so";

    m_logFileName = QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss");

    m_emsInfo = new EmsInfoView();
    connect(m_emsInfo, SIGNAL(displayLocationId(int, bool, FormatType)), this, SLOT(emsInfoDisplayLocationId(int, bool, FormatType)));

    m_emsMdiWindow = ui.mdiArea->addSubWindow(m_emsInfo);
    m_emsMdiWindow->setGeometry(m_emsInfo->geometry());
    m_emsMdiWindow->hide();
    m_emsMdiWindow->setWindowTitle(m_emsInfo->windowTitle());

    m_firmwareDebugView = new FirmwareDebugView();

    m_firmwareDebugMdiWindow = ui.mdiArea->addSubWindow(m_firmwareDebugView);
    m_firmwareDebugMdiWindow->setGeometry(m_firmwareDebugView->geometry());
    m_firmwareDebugMdiWindow->hide();
    m_firmwareDebugMdiWindow->setWindowTitle(m_firmwareDebugView->windowTitle());

    m_parameterView = new ParameterView();
    connect(m_parameterView, SIGNAL(showTable(QString)), this, SLOT(showTable(QString)));
    m_parameterMdiWindow = ui.mdiArea->addSubWindow(m_parameterView);
    m_parameterMdiWindow->setGeometry(m_parameterView->geometry());
    m_parameterMdiWindow->hide();
    m_parameterMdiWindow->setWindowTitle(m_parameterView->windowTitle());

    m_aboutView = new AboutView();
    m_aboutView->setHash(define2string(GIT_HASH));
    m_aboutView->setCommit(define2string(GIT_COMMIT));
    m_aboutView->setDate(define2string(GIT_DATE));
    m_aboutMdiWindow = ui.mdiArea->addSubWindow(m_aboutView);
    m_aboutMdiWindow->setGeometry(m_aboutView->geometry());
    m_aboutMdiWindow->hide();
    m_aboutMdiWindow->setWindowTitle(m_aboutView->windowTitle());

    m_dataGauges = new GaugeView();
    //    connect(m_dataGauges, SIGNAL(destroyed()), this, SLOT(dataGaugesDestroyed()));
    m_gaugesMdiWindow = ui.mdiArea->addSubWindow(m_dataGauges);
    m_gaugesMdiWindow->setGeometry(m_dataGauges->geometry());
    m_gaugesMdiWindow->hide();
    m_gaugesMdiWindow->setWindowTitle(m_dataGauges->windowTitle());

    loadDashboards("src/");
    loadDashboards(".");
    loadDashboards(m_defaultsDir + "/" + "dashboards");

    if (QFile::exists(m_defaultsDir + "/" + "dashboards/gauges.qml")) {
        //qml file is in the program files directory, or in /usr/share
        m_dataGauges->setFile(m_defaultsDir + "/" + "dashboards/gauges.qml");
    } else if (QFile::exists("src/gauges.qml")) {
        //We're operating out of the src directory
        m_dataGauges->setFile("src/gauges.qml");
    } else if (QFile::exists("gauges.qml")) {
        //Running with no install, but not src?? Still handle it.
        m_dataGauges->setFile("gauges.qml");
    }

    m_firmwareMetaData = new FirmwareMetaData();
    m_firmwareMetaMdiWindow = ui.mdiArea->addSubWindow(m_firmwareMetaData);
    m_firmwareMetaMdiWindow->setGeometry(m_firmwareMetaData->geometry());
    m_firmwareMetaMdiWindow->hide();
    m_firmwareMetaMdiWindow->setWindowTitle(m_firmwareMetaData->windowTitle());

    m_dataTables = new TableView();
    m_tablesMdiWindow = ui.mdiArea->addSubWindow(m_dataTables);
    m_tablesMdiWindow->setGeometry(m_dataTables->geometry());
    m_tablesMdiWindow->hide();
    m_tablesMdiWindow->setWindowTitle(m_dataTables->windowTitle());

    m_statusView = new EmsStatus(this);
    this->addDockWidget(Qt::RightDockWidgetArea, m_statusView);
    connect(m_statusView, SIGNAL(hardResetRequest()), this, SLOT(emsStatusHardResetRequested()));
    connect(m_statusView, SIGNAL(softResetRequest()), this, SLOT(emsStatusSoftResetRequested()));

    m_dataFlags = new FlagView();
    m_dataFlags->passDecoder(m_dataPacketDecoder);
    m_flagsMdiWindow = ui.mdiArea->addSubWindow(m_dataFlags);
    m_flagsMdiWindow->setGeometry(m_dataFlags->geometry());
    m_flagsMdiWindow->hide();
    m_flagsMdiWindow->setWindowTitle(m_dataFlags->windowTitle());

    m_packetStatus = new PacketStatusView();
    m_packetStatusMdiWindow = ui.mdiArea->addSubWindow(m_packetStatus);
    m_packetStatusMdiWindow->setGeometry(m_packetStatus->geometry());
    m_packetStatusMdiWindow->hide();
    m_packetStatusMdiWindow->setWindowTitle(m_packetStatus->windowTitle());

    //Load settings
    QLOG_INFO() << "Local settings file is:" << m_settingsFile;
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup("comms");
#if defined(Q_OS_WIN)
    m_comPort = settings.value("port", "COM4").toString();
#elif defined(Q_OS_LINUX)
    m_comPort = settings.value("port", "/dev/ttyUSB0").toString();
#else
    m_comPort = settings.value("port", "/dev/cu.usbmodem").toString();
#endif
    m_comBaud = settings.value("baud", 115200).toInt();
    m_comInterByte = settings.value("interbytedelay", 0).toInt();
    m_saveLogs = settings.value("savelogs", true).toBool();
    m_clearLogs = settings.value("clearlogs", false).toBool();
    m_logsToKeep = settings.value("logstokeep", 0).toInt();
    m_logDirectory = settings.value("logdir", m_localHomeDir + "/logs").toString();
    m_debugLogs = settings.value("debuglogs", false).toBool();
    settings.endGroup();

    m_pidcount = 0;

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    m_timer->start(1000);

    m_guiUpdateTimer = new QTimer(this);
    connect(m_guiUpdateTimer, SIGNAL(timeout()), this, SLOT(guiUpdateTimerTick()));
    m_guiUpdateTimer->start(250);

    statusBar()->addWidget(ui.ppsLabel);
    statusBar()->addWidget(ui.statusLabel);
    ui.statusLabel->setAccessibleName("Status Text");
    ui.statusLabel->setAccessibleDescription("Status Text Label");

    m_logfile = new QFile("myoutput.log");
    m_logfile->open(QIODevice::ReadWrite | QIODevice::Truncate);

    //ui.menuWizards
    connect(ui.mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(subMdiWindowActivated(QMdiSubWindow *)));
    QSettings windowsettings(m_settingsFile, QSettings::IniFormat);
    windowsettings.beginGroup("general");
    this->restoreGeometry(windowsettings.value("location").toByteArray());
    if (windowsettings.value("isMaximized", false).toBool()) {
        this->showMaximized();
    }

    windowsettings.endGroup();

    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setAccessibleDescription("MainWindow");

    ui.statusLabel->setText("<font bgcolor=\"#FF0000\">DISCONNECTED</font>");
}

MainWindow::~MainWindow()
{

    //Remove all WizardView windows
    for (int i = 0; i < m_wizardList.size(); i++) {
        m_wizardList[i]->close();
        delete m_wizardList[i];
    }
    m_wizardList.clear();

    m_emsComms->stop();
    //emsComms->wait(1000);
    delete m_emsComms;
}

void MainWindow::menu_windows_interrogateProgressViewClicked()
{
    m_interrogateProgressMdiWindow->show();
    QApplication::postEvent(m_interrogateProgressMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_interrogateProgressMdiWindow, new QEvent(QEvent::WindowActivate));
}
void MainWindow::menu_windows_ParameterViewClicked()
{
    m_parameterMdiWindow->show();
    QApplication::postEvent(m_parameterMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_parameterMdiWindow, new QEvent(QEvent::WindowActivate));
}
void MainWindow::menu_windows_firmwareMetadataClicked()
{
    m_firmwareMetaMdiWindow->show();
    QApplication::postEvent(m_firmwareMetaMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_firmwareMetaMdiWindow, new QEvent(QEvent::WindowActivate));
}
void MainWindow::menu_windows_firmwareDebugClicked()
{
    m_firmwareDebugMdiWindow->show();
    QApplication::postEvent(m_firmwareDebugMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_firmwareDebugMdiWindow, new QEvent(QEvent::WindowActivate));
}

void MainWindow::menu_file_saveOfflineDataClicked()
{

    QString filename = QFileDialog::getSaveFileName(this, "Save Offline File", ".", "Offline JSON Files (*.json)");
    if (filename == "") {
        return;
    }
    QVariantMap top;
    QVariantMap flashMap;
    QVariantMap metaMap;
    QVariantMap ramMap;
    QVariantMap datatable2d;
    QVariantMap datatable3d;
    QVariantMap dataraw;
    if (!m_emsComms) {
        return;
    }
    for (int i = 0; i < m_locationIdList.size(); i++) {

        if (m_emsComms->get2DTableData(m_locationIdList[i])) {
            QVariantMap current;
            QVariantList axis;
            QVariantList data;
            for (int j = 0; j < m_emsComms->get2DTableData(m_locationIdList[i])->axis().size(); j++) {
                axis.append(m_emsComms->get2DTableData(m_locationIdList[i])->axis()[j]);
            }
            for (int j = 0; j < m_emsComms->get2DTableData(m_locationIdList[i])->values().size(); j++) {
                data.append(m_emsComms->get2DTableData(m_locationIdList[i])->values()[j]);
            }
            QLOG_DEBUG() << "2D Table Saved";
            QLOG_DEBUG() << "Axis Size:" << axis.size();
            QLOG_DEBUG() << "Data Size:" << data.size();
            current["axis"] = axis;
            current["data"] = data;
            current["title"] = m_emsComms->getMetaParser()->get2DMetaData(m_locationIdList[i]).tableTitle;
            current["xtitle"] = m_emsComms->getMetaParser()->get2DMetaData(m_locationIdList[i]).xAxisTitle;
            current["ytitle"] = m_emsComms->getMetaParser()->get2DMetaData(m_locationIdList[i]).yAxisTitle;
            current["ram"] = m_emsComms->get2DTableData(m_locationIdList[i])->isRam();
            datatable2d[QString::number(m_locationIdList[i], 16).toUpper()] = current;
        }
        if (m_emsComms->get3DTableData(m_locationIdList[i])) {
            QVariantMap current;
            QVariantList xlist;
            QVariantList ylist;
            QVariantList zlist;
            for (int j = 0; j < m_emsComms->get3DTableData(m_locationIdList[i])->xAxis().size(); j++) {
                xlist.append(m_emsComms->get3DTableData(m_locationIdList[i])->xAxis()[j]);
            }
            for (int j = 0; j < m_emsComms->get3DTableData(m_locationIdList[i])->yAxis().size(); j++) {
                ylist.append(m_emsComms->get3DTableData(m_locationIdList[i])->yAxis()[j]);
            }
            for (int j = 0; j < m_emsComms->get3DTableData(m_locationIdList[i])->values().size(); j++) {
                QVariantList zrow;
                for (int k = 0; k < m_emsComms->get3DTableData(m_locationIdList[i])->values()[j].size(); k++) {
                    zrow.append(m_emsComms->get3DTableData(m_locationIdList[i])->values()[j][k]);
                }
                zlist.append((QVariant)zrow);
            }
            current["x"] = xlist;
            current["y"] = ylist;
            current["z"] = zlist;
            current["title"] = m_emsComms->getMetaParser()->get3DMetaData(m_locationIdList[i]).tableTitle;
            current["xtitle"] = m_emsComms->getMetaParser()->get3DMetaData(m_locationIdList[i]).xAxisTitle;
            current["ytitle"] = m_emsComms->getMetaParser()->get3DMetaData(m_locationIdList[i]).yAxisTitle;
            current["ztitle"] = m_emsComms->getMetaParser()->get3DMetaData(m_locationIdList[i]).zAxisTitle;
            current["ram"] = m_emsComms->get3DTableData(m_locationIdList[i])->isRam();
            datatable3d[QString::number(m_locationIdList[i], 16).toUpper()] = current;
        }
        if (m_emsComms->getRawData(m_locationIdList[i])) {
            QVariantList raw;
            for (int j = 0; j < m_emsComms->getRawData(m_locationIdList[i])->data().size(); j++) {
                raw.append((unsigned char)m_emsComms->getRawData(m_locationIdList[i])->data()[j]);
            }
            dataraw[QString::number(m_locationIdList[i], 16).toUpper()] = raw;
        }
    }
    top["2D"] = datatable2d;
    top["3D"] = datatable3d;
    top["RAW"] = dataraw;
    QJsonDocument doc = QJsonDocument::fromVariant(top);

    QFile outfile2(filename);
    outfile2.open(QIODevice::ReadWrite | QIODevice::Truncate);
    outfile2.write(doc.toJson());
    outfile2.flush();
    outfile2.close();
    /*	QJson::Serializer serializer2;
    QByteArray out2 = serializer2.serialize(top);
    QFile outfile2(filename);
    outfile2.open(QIODevice::ReadWrite | QIODevice::Truncate);
    outfile2.write(out2);
    outfile2.flush();
    outfile2.close();*/
}

void MainWindow::menu_file_loadOfflineDataClicked()
{

    QString filename = QFileDialog::getOpenFileName(this, "Load Offline File", ".", "Offline JSON Files (*.json)");
    if (filename == "") {
        QLOG_ERROR() << "No offline file selected!";
        return;
    }
    //QByteArray out = serializer.serialize(top);
    QFile outfile(filename);
    outfile.open(QIODevice::ReadOnly);
    QByteArray out = outfile.readAll();
    outfile.close();

    //Load a new instance of the plugin

    QPluginLoader *tmppluginLoader = new QPluginLoader(this);
    tmppluginLoader->setFileName(m_pluginFileName);
    QLOG_INFO() << tmppluginLoader->metaData();
    /*for (QJsonObject::const_iterator i = pluginLoader->metaData().constBegin();i!=pluginLoader->metaData().constEnd();i++)
    {
        qDebug() << i.key() << i.value();
    }*/

    QLOG_INFO() << "Attempting to load plugin:" << m_pluginFileName;
    if (!tmppluginLoader->load()) {

        QLOG_ERROR() << "Unable to load plugin. error:" << tmppluginLoader->errorString();
        exit(-1);
    }
    m_emsCommsOffline = qobject_cast<EmsComms *>(tmppluginLoader->instance());
    if (!m_emsCommsOffline) {
        QLOG_ERROR() << "Unable to load plugin!!!";
        QLOG_ERROR() << tmppluginLoader->errorString();
        exit(-1);
    }
    m_emsCommsOffline->passLogger(&QsLogging::Logger::instance());

    QJsonDocument doc = QJsonDocument::fromJson(out);
    QJsonObject top = doc.object();
    QJsonObject datatable2d = top.value("2D").toObject();
    for (QJsonObject::const_iterator i = datatable2d.constBegin(); i != datatable2d.constEnd(); i++) {
        //        int locid = i.key().toInt();
        //        QJsonObject data = i.value().toObject();
        //        QJsonArray axislist = data["axis"].toArray();
        //        QJsonArray datalist = data["data"].toArray();
        //        Table2DData* datar = m_emsCommsOffline->get2DTableData(locid);
    }

    /*	QJson::Parser parser;
    bool ok = false;
    QVariant outputvar = parser.parse(out,&ok);
    if (!ok)
    {
        QLOG_ERROR() << "Error parsing json:" << parser.errorString();
        return;
    }

    QVariantMap top = outputvar.toMap();
    QVariantMap datatable2d = top["2D"].toMap();
    QVariantMap datatable3d = top["3D"].toMap();
    QVariantMap dataraw = top["RAW"].toMap();

    for(QVariantMap::const_iterator i=datatable2d.constBegin();i!=datatable2d.constEnd();i++)
    {
        bool ok = false;
        unsigned short locid = i.key().toInt(&ok,16);
        if (!ok)
        {
            QMessageBox::information(0,"Error","Error parsing json");
            return;
        }
        QVariantMap tablemap = i.value().toMap();
        QVariantList axislist = tablemap["axis"].toList();
        QVariantList datalist = tablemap["data"].toList();
        Table2DData *data = emsComms->get2DTableData(locid);
        data->setWritesEnabled(false);
        for (int j=0;j<axislist.size();j++)
        {
            data->setCell(0,j,axislist[j].toDouble());

        }
        for (int j=0;j<datalist.size();j++)
        {
            data->setCell(1,j,datalist[j].toDouble());
        }
        data->setWritesEnabled(true);
        if (tablemap["ram"].toBool())
        {
            data->writeWholeLocation(true);
        }
        else
        {
            data->writeWholeLocation(false);
        }
    }
    for (QVariantMap::const_iterator i=datatable3d.constBegin();i!=datatable3d.constEnd();i++)
    {
        bool ok = false;
        unsigned short locid = i.key().toInt(&ok,16);
        if (!ok)
        {
            QMessageBox::information(0,"Error","Error parsing json");
            return;
        }
        QVariantMap current = i.value().toMap();
        QVariantList xlist = current["x"].toList();
        QVariantList ylist = current["y"].toList();
        QVariantList zlist = current["z"].toList();
        Table3DData *data = emsComms->get3DTableData(locid);
        data->setWritesEnabled(false);
        for (int j=0;j<xlist.size();j++)
        {
            data->setCell(ylist.size()-1,j,xlist[j].toDouble());
        }
        for (int j=0;j<ylist.size();j++)
        {
            data->setCell(j,0,ylist[j].toDouble());
        }
        for (int j=0;j<zlist.size();j++)
        {
            QVariantList z2list = zlist[j].toList();
            for (int k=0;k<z2list.size();k++)
            {
                data->setCell(j,k,z2list[k].toDouble());
            }

        }
        data->setWritesEnabled(true);
        if (current["ram"].toBool())
        {
            data->writeWholeLocation(true);
        }
        else
        {
            data->writeWholeLocation(false);
        }

    }
    for (QVariantMap::const_iterator i=dataraw.constBegin();i!=datatable3d.constEnd();i++)
    {
        bool ok = false;
        unsigned short locid = i.key().toInt(&ok,16);
        if (!ok)
        {
            QMessageBox::information(0,"Error","Error parsing json");
            return;
        }
        QVariantList bytes = i.value().toList();
        RawData *data = emsComms->getRawData(locid);
        QByteArray bytearray;
        for (int j=0;j<bytes.size();j++)
        {
            bytearray.append(bytes[j].toUInt());
        }
        data->setData(locid,true,bytearray);
    }*/
}
void MainWindow::emsCommsSilence(qint64 lasttime)
{
    //This is called when the ems has been silent for 5 seconds, when it was previously talking.
    QLOG_WARN() << "EMS HAS GONE SILENT";
    ui.statusLabel->setStyleSheet("background-color: rgb(255, 0, 0);");
    ui.statusLabel->setText("EMS SILENT");
    m_emsSilenceTimer->start(250);
    QMessageBox::information(this, "Warning", "ECU has gone silent. If this is unintentional, it may be a sign that something is wrong...");
    m_emsSilentLastTime = lasttime;
}
void MainWindow::emsCommsSilenceTimerTick()
{
    if (m_emsSilenceLabelIsRed) {
        m_emsSilenceLabelIsRed = false;
        ui.statusLabel->setStyleSheet("color: rgb(255, 255, 0);\nbackground-color: rgb(255, 85, 0);");
        ui.statusLabel->update();
    } else {
        m_emsSilenceLabelIsRed = true;
        ui.statusLabel->setStyleSheet("color: rgb(255, 255, 0);\nbackground-color: rgb(255, 0, 0);");
        ui.statusLabel->update();
    }
}

void MainWindow::emsCommsSilenceBroken()
{
    //This is called when ems had previously been talking, gone silent, then started talking again.
    ui.statusLabel->setText("<font bgcolor=\"#00FF00\">Status Normal</font>");
    m_emsSilenceTimer->stop();
    QLOG_WARN() << "EMS HAS GONE NOISEY" << QDateTime::currentMSecsSinceEpoch() - m_emsSilentLastTime << "milliseconds of silence";
    ui.statusLabel->setStyleSheet("");
}

void MainWindow::emsCommsDisconnected()
{
    ui.actionSave_Offline_Data->setEnabled(false);
    ui.actionLoad_Offline_Data->setEnabled(false);

    ui.actionConnect->setEnabled(true);
    ui.actionDisconnect->setEnabled(false);
    m_offlineMode = true;
    ui.statusLabel->setText("<font bgcolor=\"#FF0000\">DISCONNECTED</font>");
}

void MainWindow::setPlugin(QString plugin)
{
    m_pluginFileName = plugin;
    if (m_emsComms) {
        m_emsComms->stop();
        //emsComms->terminate();
        //emsComms->wait(250); //Join it, fixes a race condition where the thread deletes before it's finished.
        m_emsComms->deleteLater();
    }
    m_pluginLoader->unload();
    m_pluginLoader->deleteLater();
    m_pluginLoader = 0;
    m_pluginLoader = new QPluginLoader(this);
    m_pluginLoader->setFileName(m_pluginFileName);
    QLOG_INFO() << m_pluginLoader->metaData();
    /*for (QJsonObject::const_iterator i = pluginLoader->metaData().constBegin();i!=pluginLoader->metaData().constEnd();i++)
    {
        qDebug() << i.key() << i.value();
    }*/

    QLOG_INFO() << "Attempting to load plugin:" << m_pluginFileName;
    if (!m_pluginLoader->load()) {

        QLOG_ERROR() << "Unable to load plugin. error:" << m_pluginLoader->errorString();
        exit(-1);
    }
    m_emsComms = qobject_cast<EmsComms *>(m_pluginLoader->instance());
    if (!m_emsComms) {
        QLOG_ERROR() << "Unable to load plugin!!!";
        QLOG_ERROR() << m_pluginLoader->errorString();
        exit(-1);
    }
    m_emsComms->passLogger(&QsLogging::Logger::instance());

    QStringList searchpaths;
    searchpaths.append(m_settingsDir + "/" + "definitions");
    searchpaths.append(m_defaultsDir + "/definitions");
    searchpaths.append("."); //Local
    searchpaths.append("../../.."); //OSX local
    m_memoryMetaData = m_emsComms->getMetaParser();
    //m_memoryMetaData->loadMetaDataFromFile(searchpaths); //Changed to trigger a load from a file found internally.
    //emsData->setMetaData(m_memoryMetaData);
    m_parameterView->passConfigBlockList(m_memoryMetaData->configMetaData());
    m_parameterView->passMenuList(m_memoryMetaData->menuMetaData());
    QLOG_INFO() << m_memoryMetaData->errorMap().keys().size() << "Error Keys Loaded";
    QLOG_INFO() << m_memoryMetaData->table3DMetaData().size() << "3D Tables Loaded";
    QLOG_INFO() << m_memoryMetaData->table2DMetaData().size() << "2D Tables Loaded";
    m_dataPacketDecoder = m_emsComms->getDecoder();
    //connect(dataPacketDecoder,SIGNAL(payloadDecoded(QVariantMap)),this,SLOT(dataLogDecoded(QVariantMap)));
    connect(m_emsComms, SIGNAL(dataLogPayloadDecoded(QVariantMap)), this, SLOT(dataLogDecoded(QVariantMap)));
    m_dataTables->passDecoder(m_dataPacketDecoder);
    m_logFileName = QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss");
    m_emsComms->setLogFileName(m_logFileName);
    m_emsComms->setLogDirectory(m_logDirectory);
    connect(m_emsComms, SIGNAL(resetDetected(int)), this, SLOT(ecuResetDetected(int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(dataLogPayloadDecoded(QVariantMap)), this, SLOT(dataLogDecoded(QVariantMap)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(interrogationProgress(int, int)), this, SLOT(interrogationProgress(int, int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(interrogationComplete()), this, SLOT(interrogationComplete()), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(interrogateTaskStart(QString, int)), this, SLOT(interrogateTaskStart(QString, int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(interrogateTaskSucceed(int)), this, SLOT(interrogateTaskSucceed(int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(interrogateTaskFail(int)), this, SLOT(interrogateTaskFail(int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(connected()), this, SLOT(emsCommsConnected()), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(emsSilenceStarted(qint64)), this, SLOT(emsCommsSilence(qint64)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(emsSilenceBroken()), this, SLOT(emsCommsSilenceBroken()), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(error(QString)), this, SLOT(error(QString)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(error(SerialPortStatus, QString)), this, SLOT(error(SerialPortStatus, QString)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(disconnected()), this, SLOT(emsCommsDisconnected()), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(dataLogPayloadReceived(QByteArray, QByteArray)), this, SLOT(logPayloadReceived(QByteArray, QByteArray)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(locationIdList(QList<unsigned short>)), this, SLOT(locationIdList(QList<unsigned short>)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(unknownPacket(QByteArray, QByteArray)), this, SLOT(unknownPacket(QByteArray, QByteArray)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(commandSuccessful(int)), this, SLOT(commandSuccessful(int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(commandTimedOut(int)), this, SLOT(commandTimedOut(int)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(commandFailed(int, unsigned short)), this, SLOT(commandFailed(int, unsigned short)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(locationIdInfo(unsigned short, MemoryLocationInfo)), this, SLOT(locationIdInfo(unsigned short, MemoryLocationInfo)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(packetSent(unsigned short, QByteArray, QByteArray)), m_packetStatus, SLOT(passPacketSent(unsigned short, QByteArray, QByteArray)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(packetAcked(unsigned short, QByteArray, QByteArray)), m_packetStatus, SLOT(passPacketAck(unsigned short, QByteArray, QByteArray)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(packetNaked(unsigned short, QByteArray, QByteArray, unsigned short)), m_packetStatus, SLOT(passPacketNak(unsigned short, QByteArray, QByteArray, unsigned short)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(decoderFailure(QByteArray)), m_packetStatus, SLOT(passDecoderFailure(QByteArray)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(interrogationData(QMap<QString, QString>)), this, SLOT(interrogationData(QMap<QString, QString>)), Qt::QueuedConnection);
    connect(m_emsComms, SIGNAL(memoryDirty()), m_statusView, SLOT(setEmsMemoryDirty()));
    connect(m_emsComms, SIGNAL(memoryClean()), m_statusView, SLOT(setEmsMemoryClean()));
    connect(m_emsComms, SIGNAL(datalogDescriptor(QString)), this, SLOT(datalogDescriptor(QString)));
    connect(m_emsComms, SIGNAL(ramLocationDirty(unsigned short)), this, SLOT(ramLocationDirty(unsigned short)));
    connect(m_emsComms, SIGNAL(flashLocationDirty(unsigned short)), this, SLOT(flashLocationDirty(unsigned short)));
    connect(m_emsComms, SIGNAL(firmwareDebugReceived(QString)), m_firmwareDebugView, SLOT(firmwareMessage(QString)));
    connect(m_emsComms, SIGNAL(deviceFlashLocationNotSynced(unsigned short)), this, SLOT(deviceRamLocationOutOfSync(unsigned short)));
    m_emsComms->setBaud(m_comBaud);
    m_emsComms->setPort(m_comPort);
    m_emsComms->setLogsEnabled(m_saveLogs);
    m_emsComms->setInterByteSendDelay(m_comInterByte);
    m_emsComms->setlogsDebugEnabled(m_debugLogs);
    //emsComms->start();
}

void MainWindow::locationIdList(QList<unsigned short> idlist)
{
    m_locationIdList = idlist;
}

void MainWindow::menu_enableDatalogsClicked()
{
    if (m_emsComms) {
        m_emsComms->enableDatalogStream();
    }
}

void MainWindow::menu_disableDatalogsClicked()
{
    if (m_emsComms) {
        m_emsComms->disableDatalogStream();
    }
}

void MainWindow::setDevice(QString dev)
{
    m_comPort = dev;
    if (m_emsComms) {
        m_emsComms->setPort(dev);
    }
}

void MainWindow::connectToEms()
{
    menu_connectClicked();
}

void MainWindow::menu_aboutClicked()
{
    m_aboutMdiWindow->show();
    QApplication::postEvent(m_aboutMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_aboutMdiWindow, new QEvent(QEvent::WindowActivate));
}

void MainWindow::menu_windows_PacketStatusClicked()
{
    m_packetStatusMdiWindow->show();
    QApplication::postEvent(m_packetStatusMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_packetStatusMdiWindow, new QEvent(QEvent::WindowActivate));
}
void MainWindow::showTable(QString table)
{
    Table2DData *data2d = m_emsComms->get2DTableData(table);
    Table3DData *data3d = m_emsComms->get3DTableData(table);
    if (data2d) {
        TableView2D *view = new TableView2D();
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));
        QString title;
        //Table2DMetaData metadata = m_memoryMetaData->get2DMetaData(locid);
        //view->setMetaData(metadata);
        DataBlock *block = dynamic_cast<DataBlock *>(data2d);
        if (!view->setData(table, block)) {
            return;
        }
        //title = metadata.tableTitle;
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));

        QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
        //win->setWindowTitle("Ram Location 0x" + QString::number(locid,16).toUpper() + " " + title);
        win->setWindowTitle("Ram Location " + table);
        win->setGeometry(0, 0, ((view->width() < this->width() - 160) ? view->width() : this->width() - 160), ((view->height() < this->height() - 100) ? view->height() : this->height() - 100));
        //m_rawDataView[locid] = view;
        win->show();
        QApplication::postEvent(win, new QEvent(QEvent::Show));
        QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
    } else if (data3d) {
        TableView3D *view = new TableView3D();
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));
        QString title;
        //Table2DMetaData metadata = m_memoryMetaData->get2DMetaData(locid);
        //view->setMetaData(metadata);
        DataBlock *block = dynamic_cast<DataBlock *>(data3d);
        if (!view->setData(table, block)) {
            return;
        }
        //title = metadata.tableTitle;
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));

        QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
        //win->setWindowTitle("Ram Location 0x" + QString::number(locid,16).toUpper() + " " + title);
        win->setWindowTitle("Ram Location " + table);
        win->setGeometry(0, 0, ((view->width() < this->width() - 160) ? view->width() : this->width() - 160), ((view->height() < this->height() - 100) ? view->height() : this->height() - 100));
        //m_rawDataView[locid] = view;
        win->show();
        QApplication::postEvent(win, new QEvent(QEvent::Show));
        QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
    }
}

void MainWindow::createView(unsigned short locid, FormatType type)
{
    if (type == TABLE_3D) {
        Table3DData *data = m_emsComms->get3DTableData(locid);
        TableView3D *view = new TableView3D();
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));
        connect(view, SIGNAL(show3DTable(unsigned short, Table3DData *)), this, SLOT(tableview3d_show3DTable(unsigned short, Table3DData *)));
        QString title;
        Table3DMetaData metadata = m_memoryMetaData->get3DMetaData(locid);
        view->setMetaData(metadata);
        DataBlock *block = dynamic_cast<DataBlock *>(data);
        if (!view->setData(locid, block)) {
            QMessageBox::information(0, "Error", "Table view contains invalid data! Please check your firmware");
            view->deleteLater();
            return;
        }
        title = metadata.tableTitle;
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));

        QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
        win->setWindowTitle("Ram Location 0x" + QString::number(locid, 16).toUpper() + " " + title);
        win->setGeometry(0, 0, ((view->width() < this->width() - 160) ? view->width() : this->width() - 160), ((view->height() < this->height() - 100) ? view->height() : this->height() - 100));
        m_rawDataView[locid] = view;
        win->show();
        QApplication::postEvent(win, new QEvent(QEvent::Show));
        QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
    } else if (type == TABLE_2D_STRUCTURED || type == TABLE_2D_STRUCTURED || type == TABLE_2D_STRUCTURED) {
        Table2DData *data = m_emsComms->get2DTableData(locid);
        TableView2D *view = new TableView2D();
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));
        QString title;
        Table2DMetaData metadata = m_memoryMetaData->get2DMetaData(locid);
        view->setMetaData(metadata);
        DataBlock *block = dynamic_cast<DataBlock *>(data);
        if (!view->setData(locid, block)) {
            return;
        }
        title = metadata.tableTitle;
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));

        QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
        win->setWindowTitle("Ram Location 0x" + QString::number(locid, 16).toUpper() + " " + title);
        win->setGeometry(0, 0, ((view->width() < this->width() - 160) ? view->width() : this->width() - 160), ((view->height() < this->height() - 100) ? view->height() : this->height() - 100));
        m_rawDataView[locid] = view;
        win->show();
        QApplication::postEvent(win, new QEvent(QEvent::Show));
        QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
    } else {
        //Unhandled data type. Show it as a hex view.
        RawData *data = m_emsComms->getRawData(locid);
        RawDataView *view = new RawDataView(!data->isFlashOnly(), true);
        connect(view, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));
        view->setData(locid, data);

        QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
        connect(win, SIGNAL(destroyed(QObject *)), this, SLOT(rawDataViewDestroyed(QObject *)));
        win->setWindowTitle("Ram Location 0x" + QString::number(locid, 16).toUpper());
        win->setGeometry(0, 0, ((view->width() < this->width() - 160) ? view->width() : this->width() - 160), ((view->height() < this->height() - 100) ? view->height() : this->height() - 100));
        m_rawDataView[locid] = view;
        win->show();
        QApplication::postEvent(win, new QEvent(QEvent::Show));
        QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
    }
}

void MainWindow::emsInfoDisplayLocationId(int locid, bool isram, FormatType type)
{
    Q_UNUSED(isram)
    if (!m_rawDataView.contains(locid)) {
        createView(locid, type);
    } else {
        QApplication::postEvent(m_rawDataView[locid]->parent(), new QEvent(QEvent::Show));
        QApplication::postEvent(m_rawDataView[locid]->parent(), new QEvent(QEvent::WindowActivate));
    }
}

void MainWindow::interrogateProgressViewDestroyed(QObject *object)
{
    Q_UNUSED(object);
    if (m_interrogationInProgress) {
        m_interrogationInProgress = false;
        interrogateProgressViewCancelClicked();
    }
    QMdiSubWindow *win = qobject_cast<QMdiSubWindow *>(object->parent());
    if (!win) {
        return;
    }
    win->hide();
}

void MainWindow::rawDataViewDestroyed(QObject *object)
{
    QMap<unsigned short, QWidget *>::const_iterator i = m_rawDataView.constBegin();
    while (i != m_rawDataView.constEnd()) {
        if (i.value() == object) {
            //This is the one that needs to be removed.
            m_rawDataView.remove(i.key());
            QMdiSubWindow *win = qobject_cast<QMdiSubWindow *>(object->parent());
            if (!win) {
                win = qobject_cast<QMdiSubWindow *>(object);
                if (!win) {
                    return;
                }
            }
            ui.menuOpen_Windows->removeAction(m_mdiSubWindowToActionMap[win]);
            m_mdiSubWindowToActionMap.remove(win);
            win->hide();
            ui.mdiArea->removeSubWindow(win);
            return;
        }
        i++;
    }
}

void MainWindow::ui_saveDataButtonClicked()
{
}

void MainWindow::menu_settingsClicked()
{
    ComSettings *settings = new ComSettings();
    //connect(settings,SIGNAL(windowHiding(QMdiSubWindow*)),this,SLOT(windowHidden(QMdiSubWindow*)));
    settings->setComPort(m_comPort);
    settings->setBaud(m_comBaud);
    settings->setSaveDataLogs(m_saveLogs);
    settings->setClearDataLogs(m_clearLogs);
    settings->setNumLogsToSave(m_logsToKeep);
    settings->setDataLogDir(m_logDirectory);
    settings->setInterByteDelay(m_comInterByte);
    connect(settings, SIGNAL(saveClicked()), this, SLOT(settingsSaveClicked()));
    connect(settings, SIGNAL(cancelClicked()), this, SLOT(settingsCancelClicked()));
    QMdiSubWindow *win = ui.mdiArea->addSubWindow(settings);
    win->setWindowTitle(settings->windowTitle());
    win->setGeometry(settings->geometry());
    win->show();
    settings->show();
}

void MainWindow::menu_connectClicked()
{
    if (m_interrogateProgressMdiWindow) {
        delete m_interrogateProgressMdiWindow;
        m_progressView = 0;
        m_interrogateProgressMdiWindow = 0;
    }

    if (!m_offlineMode) {
        //emsData->clearAllMemory();
    }
    m_emsSilenceTimer->stop();
    m_emsInfo->clear();
    ui.actionConnect->setEnabled(false);
    ui.actionDisconnect->setEnabled(true);
    m_interrogationInProgress = true;

    //m_tempMemoryList.clear();
    m_interrogationSequenceList.clear();
    m_locIdMsgList.clear();
    m_locIdInfoMsgList.clear();
    m_emsMdiWindow->hide();
    QList<QWidget *> toDeleteList;
    for (QMap<unsigned short, QWidget *>::const_iterator i = m_rawDataView.constBegin(); i != m_rawDataView.constEnd(); i++) {
        toDeleteList.append(i.value());
        //delete (*i);
    }
    for (int i = 0; i < toDeleteList.size(); i++) {
        delete toDeleteList[i];
    }
    m_rawDataView.clear();
    if (!m_emsComms) {
        QLOG_ERROR() << "No EMSCOMMS!!!";
    }
    QLOG_INFO() << "Starting emsComms:" << m_emsComms;
    //emsComms->start();
    m_emsComms->connectSerial(m_comPort, m_comBaud);
}

void MainWindow::menu_disconnectClicked()
{
    m_emsComms->disconnectSerial();
    m_emsSilenceTimer->start();
}

void MainWindow::timerTick()
{
    ui.ppsLabel->setText("PPS: " + QString::number(m_pidcount));
    m_pidcount = 0;
}
void MainWindow::settingsSaveClicked()
{
    ComSettings *comSettingsWidget = qobject_cast<ComSettings *>(sender());
    m_comBaud = comSettingsWidget->getBaud();
    m_comPort = comSettingsWidget->getComPort();
    m_comInterByte = comSettingsWidget->getInterByteDelay();
    m_saveLogs = comSettingsWidget->getSaveDataLogs();
    m_clearLogs = comSettingsWidget->getClearDataLogs();
    m_logsToKeep = comSettingsWidget->getNumLogsToSave();
    m_logDirectory = comSettingsWidget->getDataLogDir();

    comSettingsWidget->hide();
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup("comms");
    settings.setValue("port", m_comPort);
    settings.setValue("baud", m_comBaud);
    settings.setValue("interbytedelay", m_comInterByte);
    settings.setValue("savelogs", m_saveLogs);
    settings.setValue("clearlogs", m_clearLogs);
    settings.setValue("logstokeep", m_logsToKeep);
    settings.setValue("logdir", m_logDirectory);
    settings.setValue("debuglogs", m_debugLogs);
    settings.endGroup();
    QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow *>(comSettingsWidget->parent());
    ui.mdiArea->removeSubWindow(subwin);
    comSettingsWidget->close();
    comSettingsWidget->deleteLater();
    if (m_emsComms) {
        m_emsComms->setInterByteSendDelay(m_comInterByte);
        m_emsComms->setlogsDebugEnabled(m_debugLogs);
        m_emsComms->setLogDirectory(m_logDirectory);
    }
}
void MainWindow::locationIdInfo(unsigned short locationid, MemoryLocationInfo info)
{
    if (m_memoryInfoMap.contains(locationid)) {
        QLOG_WARN() << "Duplicate location ID recieved from ECU:"
                    << "0x" + QString::number(locationid, 16).toUpper();
        //return;
    }
    m_memoryInfoMap[locationid] = info;
    QString title = "";
    if (info.metaData.valid) {
        title = info.metaData.name;
    } else {
        title = "Unknown";
    }
    /*if (m_memoryMetaData->has2DMetaData(locationid))
    {
        //title = m_memoryMetaData->get2DMetaData(locationid).tableTitle;
        if (m_memoryMetaData->get2DMetaData(locationid).size != info.size)
        {
            interrogateProgressViewCancelClicked();
            QMessageBox::information(0,"Interrogate Error","Error: Meta data for table location 0x" + QString::number(locationid,16).toUpper() + " is not valid for actual table. Size: " + QString::number(info.size) + " expected: " + QString::number(m_memoryMetaData->get2DMetaData(locationid).size));
        }
    }
    if (m_memoryMetaData->has3DMetaData(locationid))
    {
        //title = m_memoryMetaData->get3DMetaData(locationid).tableTitle;
        if (m_memoryMetaData->get3DMetaData(locationid).size != info.size)
        {
            interrogateProgressViewCancelClicked();
            QMessageBox::information(0,"Interrogate Error","Error: Meta data for table location 0x" + QString::number(locationid,16).toUpper() + " is not valid for actual table. Size: " + QString::number(info.size) + " expected: " + QString::number(m_memoryMetaData->get3DMetaData(locationid).size));
        }
    }
    if (m_memoryMetaData->hasRORMetaData(locationid))
    {
        //title = m_memoryMetaData->getRORMetaData(locationid).dataTitle;
        //m_readOnlyMetaDataMap[locationid]
    }
    if (m_memoryMetaData->hasLookupMetaData(locationid))
    {
        //title = m_memoryMetaData->getLookupMetaData(locationid).title;
    }*/
    //emsInfo->locationIdInfo(locationid,title,rawFlags,flags,parent,rampage,flashpage,ramaddress,flashaddress,size);
    m_emsInfo->locationIdInfo(locationid, title, info);
    //emsData->passLocationInfo(locationid,info);

    //We don't care about ram only locations, since they're not available in offline mode anyway.
    if (info.isFlash) {
        //checkEmsData->passLocationInfo(locationid,info);
    }
}

void MainWindow::settingsCancelClicked()
{
    //comSettings->hide();
    ComSettings *comSettingsWidget = qobject_cast<ComSettings *>(sender());
    comSettingsWidget->hide();
    QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow *>(comSettingsWidget->parent());
    ui.mdiArea->removeSubWindow(subwin);
    comSettingsWidget->close();
    comSettingsWidget->deleteLater();
}
void MainWindow::menu_windows_EmsStatusClicked()
{
    m_statusView->show();
}

void MainWindow::menu_windows_GaugesClicked()
{
    m_gaugesMdiWindow->show();
    m_dataGauges->show();
    QApplication::postEvent(m_gaugesMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_gaugesMdiWindow, new QEvent(QEvent::WindowActivate));
}

void MainWindow::menu_windows_EmsInfoClicked()
{
    m_emsMdiWindow->show();
    QApplication::postEvent(m_emsMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_emsMdiWindow, new QEvent(QEvent::WindowActivate));
}

void MainWindow::menu_windows_TablesClicked()
{
    m_tablesMdiWindow->show();
    QApplication::postEvent(m_tablesMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_tablesMdiWindow, new QEvent(QEvent::WindowActivate));
}
void MainWindow::menu_windows_FlagsClicked()
{
    m_flagsMdiWindow->show();
    QApplication::postEvent(m_flagsMdiWindow, new QEvent(QEvent::Show));
    QApplication::postEvent(m_flagsMdiWindow, new QEvent(QEvent::WindowActivate));
}

void MainWindow::unknownPacket(QByteArray header, QByteArray payload)
{
    QString result = "";
    for (int i = 0; i < header.size(); i++) {
        result += (((unsigned char)header[i] < (char)0xF) ? "0" : "") + QString::number((unsigned char)header[i], 16).toUpper();
    }
    for (int i = 0; i < payload.size(); i++) {
        result += (((unsigned char)payload[i] < (char)0xF) ? "0" : "") + QString::number((unsigned char)payload[i], 16).toUpper();
    }
}

void MainWindow::loadLogButtonClicked()
{
    QFileDialog file;
    if (file.exec()) {
        if (file.selectedFiles().size() > 0) {
            QString filename = file.selectedFiles()[0];
            ui.statusLabel->setText("Status: File loaded and not playing");
            //logLoader->loadFile(filename);
            m_emsComms->loadLog(filename);
        }
    }
}
void MainWindow::interByteDelayChanged(int num)
{
    m_emsComms->setInterByteSendDelay(num);
}

void MainWindow::logFinished()
{
    ui.statusLabel->setText("Status: File loaded and log finished");
}

void MainWindow::playLogButtonClicked()
{
    //logLoader->start();
    m_emsComms->playLog();
    ui.statusLabel->setText("Status: File loaded and playing");
}
void MainWindow::interrogationData(QMap<QString, QString> datamap)
{
    m_firmwareMetaData->setInterrogationData(datamap);
    //emsInfo->setInterrogationData(datamap);
}

void MainWindow::interfaceVersion(QString version)
{
    //ui.interfaceVersionLineEdit->setText(version);
    m_interfaceVersion = version;
    if (m_emsInfo) {
    }
    m_emsinfo.interfaceVersion = version;
}
void MainWindow::firmwareVersion(QString version)
{
    //ui.firmwareVersionLineEdit->setText(version);
    m_firmwareVersion = version;
    this->setWindowTitle(QString("EMStudio ") + QString(define2string(GIT_COMMIT)) + " Firmware: " + version);
    if (m_emsInfo) {
    }
    m_emsinfo.firmwareVersion = version;
}
void MainWindow::error(SerialPortStatus error, QString msg)
{
    Q_UNUSED(error); //We don't actually use the error here, since we treat every error the same.

    switch (QMessageBox::information(0, "", msg, "Ok", "Retry", "Load Offline Data")) {
    case 0: //Ok
        //We do nothing here.
        break;
    case 1: //Retry
        //Delay of 2 seconds to allow for this function to return, and the emscomms loop to be destroyed
        if (error == SM_MODE) {
            //We need to send a SM reset, this is not supported yet.
        }
        QTimer::singleShot(500, this, SLOT(menu_connectClicked()));
        break;
    case 2: //Load Offline Data
        //Delay here, to ensure that it goes into the event loop and isn't directly called.
        QTimer::singleShot(500, this, SLOT(menu_file_loadOfflineDataClicked()));
        break;
    default:
        QLOG_FATAL() << "WEEEE";
        break;
    }
}

void MainWindow::error(QString msg)
{
    QMessageBox::information(0, "Error", msg);
}
void MainWindow::interrogateProgressViewCancelClicked()
{
    m_offlineMode = true;
}
void MainWindow::emsCompilerVersion(QString version)
{
    m_emsinfo.compilerVersion = version;
}

void MainWindow::emsFirmwareBuildDate(QString date)
{
    m_emsinfo.firmwareBuildDate = date;
}

void MainWindow::emsDecoderName(QString name)
{
    m_emsinfo.decoderName = name;
}

void MainWindow::emsOperatingSystem(QString os)
{
    m_emsinfo.operatingSystem = os;
}
void MainWindow::loadDashboards(QString dir)
{
    QDir dashboards(dir);
    foreach (QString file, dashboards.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (file.endsWith(".qml")) {
            GaugeView *view = new GaugeView();
            QMdiSubWindow *mdiView = ui.mdiArea->addSubWindow(view);
            mdiView->setGeometry(view->geometry());
            mdiView->hide();
            mdiView->setWindowTitle(view->windowTitle());
            QString plugincompat = view->setFile(dashboards.absoluteFilePath(file));
            QAction *action = new QAction(this);
            action->setText(file.mid(0, file.lastIndexOf(".")));
            action->setCheckable(true);
            ui.menuDashboards->addAction(action);
            connect(action, SIGNAL(triggered(bool)), mdiView, SLOT(setVisible(bool)));
            m_dashboardList.append(view);
            if (!m_gaugeActionMap.contains(plugincompat)) {
                m_gaugeActionMap[plugincompat] = QList<QAction *>();
            }
            m_gaugeActionMap[plugincompat].append(action);
        }
    }
}
void MainWindow::loadWizards(QString dir)
{
    QDir wizards(dir);
    foreach (QString file, wizards.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (file.endsWith(".qml")) {
            WizardView *view = new WizardView();
            connect(m_emsComms, SIGNAL(configRecieved(ConfigBlock, QVariant)), view, SLOT(configRecieved(ConfigBlock, QVariant)), Qt::QueuedConnection);
            m_wizardList.append(view);
            for (int i = 0; i < m_emsComms->getConfigList().size(); i++) {
                view->addConfig(m_emsComms->getConfigList()[i], m_emsComms->getConfigData(m_emsComms->getConfigList()[i]));
            }
            view->setFile(m_emsComms, wizards.absoluteFilePath(file));
            view->passConfig(m_memoryMetaData->configMetaData());
            //view->setGeometry(0,0,800,600);
            QAction *action = new QAction(this);
            action->setText(file.mid(0, file.lastIndexOf(".")));
            action->setCheckable(true);
            ui.menuWizards->addAction(action);
            connect(action, SIGNAL(triggered(bool)), view, SLOT(setVisible(bool)));
            connect(view, SIGNAL(visibilityChanged(bool)), action, SLOT(setChecked(bool)));
        }
    }
}

void MainWindow::emsCommsConnected()
{

    ui.statusLabel->setText("<font bgcolor=\"#FF0000\">Connected</font>");
    m_interrogationFailureCount = 0;
    ui.actionSave_Offline_Data->setEnabled(true);
    ui.actionLoad_Offline_Data->setEnabled(true);
    while (ui.menuWizards->actions().size() > 0) {
        QAction *action = ui.menuWizards->actions().takeFirst();
        ui.menuWizards->removeAction(action);
        delete action;
    }
    for (int i = 0; i < m_wizardList.size(); i++) {
        delete m_wizardList[i];
    }
    m_wizardList.clear();
    //m_defaultsDir
    //QDir defaultsdir(m_defaultsDir);

    //Load wizards from system, local, and home
    //TODO: Delay loading these
    //loadWizards(m_defaultsDir + "/wizards");
    //loadWizards("wizards");
    //loadWizards(m_localHomeDir + "/wizards");

    //for (int i=0;i<emsComms->getConfigList().size();i++)
    //{
    //	parameterView->addConfig(emsComms->getConfigList()[i],emsComms->getConfigData(emsComms->getConfigList()[i]));
    /*for (int j=0;j<m_wizardList.size();j++)
        {
            m_wizardList[j]->addConfig(emsComms->getConfigList()[i],emsComms->getConfigData(emsComms->getConfigList()[i]));
        }*/
    //}
    /*if (m_gaugeActionMap.contains(emsComms->getPluginCompat()))
    {
        for (int i=0;i<ui.menuDashboards->actions().size();i++)
        {
            if (!m_gaugeActionMap[emsComms->getPluginCompat()].contains(ui.menuDashboards->actions()[i]))
            {
                ui.menuDashboards->actions()[i]->setVisible(false);

            }
        }
    }
    else if (emsComms->getPluginCompat() != "")
    {
        for (int i=0;i<ui.menuDashboards->actions().size();i++)
        {
            ui.menuDashboards->actions()[i]->setVisible(false);
        }
    }*/
    //New log and settings file here.
    if (m_memoryInfoMap.size() == 0) {
        m_offlineMode = false;
    }
    if (m_progressView) {
        m_progressView->reset();
        ui.actionInterrogation_Progress->setEnabled(true);
    } else {
        ui.actionInterrogation_Progress->setEnabled(true);
        m_progressView = new InterrogateProgressView();
        connect(m_progressView, SIGNAL(destroyed(QObject *)), this, SLOT(interrogateProgressViewDestroyed(QObject *)));
        m_interrogateProgressMdiWindow = ui.mdiArea->addSubWindow(m_progressView);
        m_interrogateProgressMdiWindow->setWindowTitle(m_progressView->windowTitle());
        m_interrogateProgressMdiWindow->setGeometry(m_progressView->geometry());
        connect(m_progressView, SIGNAL(cancelClicked()), this, SLOT(interrogateProgressViewCancelClicked()));
        m_progressView->setMaximum(0);
    }
    m_interrogationSequenceList.clear();
    m_progressView->show();
    m_interrogateProgressMdiWindow->show();
    m_progressView->addOutput("Connected to EMS");
    m_emsComms->startInterrogation();
}
void MainWindow::interrogationProgress(int current, int total)
{
    Q_UNUSED(current);
    Q_UNUSED(total);
}

void MainWindow::interrogationComplete()
{
    m_interrogationInProgress = false;
    if (m_progressView) {
        //progressView->hide();
        QMdiSubWindow *win = qobject_cast<QMdiSubWindow *>(m_progressView->parent());
        if (win && m_interrogationFailureCount == 0) {
            win->hide();
        }
        m_progressView->done();
    }
    QLOG_INFO() << "Interrogation complete";
    if (m_interrogationFailureCount > 0) {
        if (QMessageBox::question(0, "Error", "One of the commands has failed during interrogation. This could be a sign of a more serious problem, Do you wish to continue?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes) {
            m_emsComms->disconnectSerial();
            return;
        } else {
            m_emsMdiWindow->show();
        }
    } else {
        m_emsMdiWindow->show();
    }
    bool oneShown = false; //Check to see if at least one window is visisble.
    QSettings windowsettings(m_settingsFile, QSettings::IniFormat);
    QString compat = m_emsComms->getPluginCompat();
    QString savecompat = windowsettings.value("plugincompat", "").toString();
    if (compat == savecompat && false) {
        windowsettings.sync();
        int size = windowsettings.beginReadArray("rawwindows");
        for (int i = 0; i < size; i++) {
            //createView()
            windowsettings.setArrayIndex(i);
            unsigned short locid = windowsettings.value("window").toInt();
            QString type = windowsettings.value("type").toString();
            if (type == "TableView2D") {
                createView(locid, TABLE_2D_STRUCTURED);
                QWidget *parent = (QWidget *)m_rawDataView[locid]->parent();
                parent->restoreGeometry(windowsettings.value("location").toByteArray());
            } else if (type == "TableView3D") {
                createView(locid, TABLE_3D);
                QWidget *parent = (QWidget *)m_rawDataView[locid]->parent();
                parent->restoreGeometry(windowsettings.value("location").toByteArray());
            } else if (type == "tablesMdiWindow") {
                m_tablesMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                qDebug() << "Geom:" << m_tablesMdiWindow->geometry();
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_tablesMdiWindow->setHidden(hidden);

            } else if (type == "firmwareMetaMdiWindow") {
                m_firmwareMetaMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_firmwareMetaMdiWindow->setHidden(hidden);
            } else if (type == "interrogateProgressMdiWindow") {
                // interrogateProgressMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                // interrogateProgressMdiWindow->setHidden(windowsettings.value("hidden",true).toBool());
            } else if (type == "emsMdiWindow") {
                m_emsMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_emsMdiWindow->setHidden(hidden);
            } else if (type == "flagsMdiWindow") {
                m_flagsMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_flagsMdiWindow->setHidden(hidden);
            } else if (type == "gaugesMdiWindow") {
                m_gaugesMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_gaugesMdiWindow->setHidden(hidden);
            } else if (type == "packetStatusMdiWindow") {
                m_packetStatusMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_packetStatusMdiWindow->setHidden(hidden);
            } else if (type == "aboutMdiWindow") {
                m_aboutMdiWindow->restoreGeometry(windowsettings.value("location").toByteArray());
                bool hidden = windowsettings.value("hidden", true).toBool();
                if (!hidden) {
                    oneShown = true;
                }
                m_aboutMdiWindow->setHidden(hidden);
            } else if (type == "emsStatusMdiWindow") {

            } else {
                qDebug() << "Unknown type:" << type;
            }
        }
        windowsettings.endArray();
    }
    if (!oneShown) {
        //No windows are currently shown. Show the emsMdiWindow
        m_emsMdiWindow->setHidden(false);
    }
    /*	QMdiSubWindow *tablesMdiWindow;
    QMdiSubWindow *firmwareMetaMdiWindow;
    QMdiSubWindow *interrogateProgressMdiWindow;
    QMdiSubWindow *emsMdiWindow;
    QMdiSubWindow *flagsMdiWindow;
    QMdiSubWindow *gaugesMdiWindow;
    QMdiSubWindow *packetStatusMdiWindow;
    QMdiSubWindow *aboutMdiWindow;
    QMdiSubWindow *emsStatusMdiWindow;*/

    /*for (QMap<unsigned short,QWidget*>::const_iterator i=m_rawDataView.constBegin();i!=m_rawDataView.constEnd();i++)
    {
        windowsettings.setArrayIndex(val++);
        windowsettings.value("window",i.key());
        QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow*>(i.value()->parent());
        windowsettings.value("location",subwin->saveGeometry());
        windowsettings.value("hidden",subwin->isHidden());
        windowsettings.value("type",i.value()->metaObject()->className());

    }
    windowsettings.endArray();*/
    m_parameterView->setActiveComms(m_emsComms);
}
void MainWindow::interrogateTaskStart(QString task, int sequence)
{
    if (task.contains("Location ID")) {
        m_progressView->addTask(task, sequence, 1);
    } else if (task.contains("Ram Location") || task.contains("Flash Location")) {
        m_progressView->addTask(task, sequence, 2);
    } else if (task.contains("Ecu Info")) {
        m_progressView->addTask(task, sequence, 0);
    }
}

void MainWindow::interrogateTaskSucceed(int sequence)
{
    m_progressView->taskSucceed(sequence);
}

void MainWindow::interrogateTaskFail(int sequence)
{
    m_progressView->taskFail(sequence);
}

void MainWindow::checkSyncRequest()
{
    m_emsComms->getLocationIdList(0, 0);
}
void MainWindow::tableview3d_show3DTable(unsigned short locationid, Table3DData *data)
{
    if (m_table3DMapViewMap.contains(locationid)) {
        m_table3DMapViewMap[locationid]->show();
        QApplication::postEvent(m_table3DMapViewMap[locationid], new QEvent(QEvent::Show));
        QApplication::postEvent(m_table3DMapViewMap[locationid], new QEvent(QEvent::WindowActivate));
        return;
    }

    TableMap3D *m_tableMap = new TableMap3D();

    m_table3DMapViewWidgetMap[locationid] = m_tableMap;
    m_tableMap->passData(data);
    QMdiSubWindow *win = ui.mdiArea->addSubWindow(m_tableMap);
    connect(win, SIGNAL(destroyed(QObject *)), this, SLOT(tableMap3DDestroyed(QObject *)));
    win->setGeometry(m_tableMap->geometry());
    win->setWindowTitle("0x" + QString::number(locationid, 16).toUpper());
    win->show();
    QApplication::postEvent(win, new QEvent(QEvent::Show));
    QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
    m_table3DMapViewMap[locationid] = win;
}
void MainWindow::tableMap3DDestroyed(QObject *object)
{
    Q_UNUSED(object)
    for (QMap<unsigned short, QMdiSubWindow *>::const_iterator i = m_table3DMapViewMap.constBegin(); i != m_table3DMapViewMap.constEnd(); i++) {
        if (i.value() == sender()) {
            m_table3DMapViewWidgetMap.remove(i.key());
            m_table3DMapViewMap.remove(i.key());
            return;
        }
    }
}

void MainWindow::emsStatusHardResetRequested()
{
    if (QMessageBox::information(0, "Warning", "Hard resetting the ECU will erase all changes currently in RAM, but not saved to FLASH, and restart the ECU. Are you sure you want to do this?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        QLOG_INFO() << "Attempting hard reset:" << m_emsComms->hardReset();
    }
}

void MainWindow::emsStatusSoftResetRequested()
{
    if (QMessageBox::information(0, "Warning", "Soft resetting the ECU will erase all changes currently in RAM, but not saved to FLASH, and restart the ECU. Are you sure you want to do this?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        QLOG_INFO() << "Attempting soft reset:" << m_emsComms->softReset();
    }
}

void MainWindow::commandTimedOut(int sequencenumber)
{
    QLOG_INFO() << "Command timed out:" << QString::number(sequencenumber);
    if (m_waitingForRamWriteConfirmation) {

    } else {
    }
    if (m_waitingForFlashWriteConfirmation) {
        m_waitingForFlashWriteConfirmation = false;

        //Find all windows that use that location id
        m_currentFlashLocationId = 0;
        //checkRamFlashSync();
        return;
    } else {
    }
    if (m_interrogationInProgress) {
        m_progressView->taskFail(sequencenumber);
        //If interrogation is in progress, we need to stop, since something has gone
        //horribly wrong.
        //interrogateProgressViewCancelClicked();

        m_interrogationFailureCount++;
    }
}
void MainWindow::commandSuccessful(int sequencenumber)
{
    QLOG_INFO() << "Command succesful:" << QString::number(sequencenumber);
    if (m_interrogationInProgress) {
        //if (progressView) progressView->taskSucceed(sequencenumber);
    }
}
void MainWindow::checkMessageCounters(int sequencenumber)
{
    QLOG_INFO() << "Checking message counters:" << sequencenumber << m_locIdInfoMsgList.size() << m_locIdMsgList.size() << m_interrogationSequenceList.size();
    if (m_locIdInfoMsgList.contains(sequencenumber)) {
        m_locIdInfoMsgList.removeOne(sequencenumber);
        if (m_locIdInfoMsgList.size() == 0) {
            if (m_offlineMode && m_checkEmsDataInUse) {
                //We were offline. Let's check.

                m_offlineMode = false;
            }
            QLOG_INFO() << "All Ram and Flash locations updated";
            //End of the location ID information messages.
            checkRamFlashSync();
        }
    } else {
        QLOG_INFO() << "Not in locidinfo";
    }
    if (m_locIdMsgList.contains(sequencenumber)) {
        m_locIdMsgList.removeOne(sequencenumber);
        if (m_locIdMsgList.size() == 0) {
            QLOG_INFO() << "All ID information recieved. Requesting Ram and Flash updates";
            //populateParentLists();
            /*TODO
             *QList<unsigned short>  memorylist = emsData->getTopLevelDeviceFlashLocations();
            for (int i=0;i<memorylist.size();i++)
            {
                int seq = emsComms->retrieveBlockFromFlash(memorylist[i],0,0);
                if (progressView) progressView->addTask("Getting Location ID 0x" + QString::number(memorylist[i],16).toUpper(),seq,2);
                m_locIdInfoMsgList.append(seq);
                if (progressView) progressView->setMaximum(progressView->maximum()+1);
                interrogationSequenceList.append(seq);
            }
            memorylist = emsData->getTopLevelDeviceRamLocations();
            for (int i=0;i<memorylist.size();i++)
            {
                int seq = emsComms->retrieveBlockFromRam(memorylist[i],0,0);
                if (progressView) progressView->addTask("Getting Location ID 0x" + QString::number(memorylist[i],16).toUpper(),seq,2);
                m_locIdInfoMsgList.append(seq);
                if (progressView) progressView->setMaximum(progressView->maximum()+1);
                interrogationSequenceList.append(seq);
            }*/
        }
    } else {
        QLOG_INFO() << "Not in locidmsglist";
    }
    if (m_interrogationSequenceList.contains(sequencenumber)) {
        QLOG_INFO() << "GOOD!";
        if (m_progressView) {
            m_progressView->setProgress(m_progressView->progress() + 1);
        }
        m_interrogationSequenceList.removeOne(sequencenumber);
        if (m_interrogationSequenceList.size() == 0) {
            m_interrogationInProgress = false;
            if (m_progressView) {
                //progressView->hide();
                QMdiSubWindow *win = qobject_cast<QMdiSubWindow *>(m_progressView->parent());
                if (win) {
                    win->hide();
                }
                m_progressView->done();
            }
            //this->setEnabled(true);
            QLOG_INFO() << "Interrogation complete";

            //			emsData->setInterrogation(false);

            //emsInfo->show();
            m_emsMdiWindow->show();
            //Write everything to the settings.
            QString json = "";
            json += "{";
            /*			QJson::Serializer jsonSerializer;
            QVariantMap top;
            top["firmwareversion"] = emsinfo.firmwareVersion;
            top["interfaceversion"] = emsinfo.interfaceVersion;
            top["compilerversion"] = emsinfo.compilerVersion;
            top["firmwarebuilddate"] = emsinfo.firmwareBuildDate;
            top["decodername"] = emsinfo.decoderName;
            top["operatingsystem"] = emsinfo.operatingSystem;
            top["emstudiohash"] = emsinfo.emstudioHash;
            top["emstudiocommit"] = emsinfo.emstudioCommit;

            if (m_saveLogs)
            {
                QFile *settingsFile = new QFile(m_logDirectory + "/" + m_logFileName + ".meta.json");
                settingsFile->open(QIODevice::ReadWrite);
                settingsFile->write(jsonSerializer.serialize(top));
                settingsFile->close();
            }*/
        } else {
            QLOG_INFO() << m_interrogationSequenceList.size() << "messages left to go. First one:" << m_interrogationSequenceList[0];
        }
    } else {
        QLOG_INFO() << "Not in interrogation list";
    }
}

void MainWindow::retrieveFlashLocationId(unsigned short locationid)
{
    m_emsComms->retrieveBlockFromFlash(locationid, 0, 0);
}

void MainWindow::retrieveRamLocationId(unsigned short locationid)
{
    m_emsComms->retrieveBlockFromRam(locationid, 0, 0);
}

void MainWindow::checkRamFlashSync()
{
    //emsData->populateLocalRamAndFlash();
}

void MainWindow::commandFailed(int sequencenumber, unsigned short errornum)
{
    QLOG_INFO() << "Command failed:" << QString::number(sequencenumber) << "0x" + QString::number(errornum, 16);
    if (!m_interrogationInProgress) {
        QMessageBox::information(0, "Command Failed", "Command failed with error: " + m_memoryMetaData->getErrorString(errornum));
    } else {
        //interrogateTaskFail(int) catches this case.
        //if (progressView) progressView->taskFail(sequencenumber);
        m_interrogationFailureCount++;
    }
}

void MainWindow::pauseLogButtonClicked()
{
}

void MainWindow::stopLogButtonClicked()
{
}
void MainWindow::connectButtonClicked()
{
    m_emsComms->connectSerial(m_comPort, m_comBaud);
}

void MainWindow::logProgress(qlonglong current, qlonglong total)
{
    Q_UNUSED(current)
    Q_UNUSED(total)
    //setWindowTitle(QString::number(current) + "/" + QString::number(total) + " - " + QString::number((float)current/(float)total));
}
void MainWindow::guiUpdateTimerTick()
{
}
void MainWindow::ecuResetDetected(int missedpackets)
{
    if (!m_interrogationInProgress && !m_EcuResetPopup) {
        m_EcuResetPopup = true;
        if (QMessageBox::question(this, "Error", "ECU Reset detected with " + QString::number(missedpackets) + " missed packets! Would you like to reflash data? If you do not, then EMStudio will continue with an INVALID idea of ECU memory, and you will lose any changes made.", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            m_emsComms->writeAllRamToRam();
        } else {
            QMessageBox::information(this, "Warning", "EMStudio will now continue with a corrupt version of ECU memory. The death of your engine is on your head now.");
        }
        m_EcuResetPopup = false;
    }
}

void MainWindow::dataLogDecoded(QVariantMap data)
{

    //m_valueMap = data;
    if (m_dataTables) {
        m_dataTables->passData(data);
    }
    if (m_dataGauges) {
        m_dataGauges->passData(data);
    }
    if (m_dataFlags) {
        m_dataFlags->passData(data);
    }
    if (m_statusView) {
        m_statusView->passData(data);
    }
    for (QMap<unsigned short, QWidget *>::const_iterator i = m_rawDataView.constBegin(); i != m_rawDataView.constEnd(); i++) {
        DataView *dview = dynamic_cast<DataView *>(i.value());
        if (dview) {
            dview->passDatalog(data);
        }
    }
    for (int i = 0; i < m_dashboardList.size(); i++) {
        m_dashboardList[i]->passData(data);
    }
}
void MainWindow::logPayloadReceived(QByteArray header, QByteArray payload)
{
    //All we're doing here is counting the incoming packets.
    Q_UNUSED(header)
    Q_UNUSED(payload)
    m_pidcount++;
}
void MainWindow::windowHidden(QMdiSubWindow *window)
{
    if (window && m_mdiSubWindowToActionMap.contains(window)) {
        ui.menuOpen_Windows->removeAction(m_mdiSubWindowToActionMap[window]);
        m_mdiSubWindowToActionMap.remove(window);
    }
}
void MainWindow::windowDestroyed(QObject *window)
{
    if (window && m_mdiSubWindowToActionMap.contains((QMdiSubWindow *)window)) {
        ui.menuOpen_Windows->removeAction(m_mdiSubWindowToActionMap[(QMdiSubWindow *)window]);
        m_mdiSubWindowToActionMap.remove((QMdiSubWindow *)window);
    }
}

void MainWindow::bringToFrontAndShow()
{
    for (QMap<QMdiSubWindow *, QAction *>::const_iterator i = m_mdiSubWindowToActionMap.constBegin(); i != m_mdiSubWindowToActionMap.constEnd(); i++) {
        if (i.value() == sender()) {
            i.key()->show();
            QApplication::postEvent(i.key(), new QEvent(QEvent::Show));
            QApplication::postEvent(i.key(), new QEvent(QEvent::WindowActivate));
        }
    }
}

void MainWindow::subMdiWindowActivated(QMdiSubWindow *window)
{
    if (window && !m_mdiSubWindowToActionMap.contains(window)) {
        if (window->isVisible()) {
            //connect(window,SIGNAL(windowStateChanged(Qt::WindowStates,Qt::WindowStates))
            QAction *action = ui.menuOpen_Windows->addAction(window->windowTitle());
            connect(action, SIGNAL(triggered()), this, SLOT(bringToFrontAndShow()));
            connect(window->widget(), SIGNAL(windowHiding(QMdiSubWindow *)), this, SLOT(windowHidden(QMdiSubWindow *)));
            connect(window, SIGNAL(destroyed(QObject *)), this, SLOT(windowDestroyed(QObject *)));
            m_mdiSubWindowToActionMap[window] = action;
        }
        //connect(action,SIGNAL(triggered(bool)),window,SLOT(setVisible(bool)));
        //connect(action,SIGNAL(triggered()),this,SLOT(bringToFrontAndShow()));
        QLOG_DEBUG() << "Window Activated New:" << window->windowTitle();
    } else if (window) {
        QLOG_DEBUG() << "Window Activated Old:" << window->windowTitle();
    }
}

void MainWindow::emsMemoryDirty()
{
}

void MainWindow::emsMemoryClean()
{
}
void MainWindow::datalogDescriptor(QString data)
{
    Q_UNUSED(data)
}
void MainWindow::ramLocationDirty(unsigned short locationid)
{
    if (!m_ramDiffWindow) {
        m_ramDiffWindow = new RamDiffWindow();
        connect(m_ramDiffWindow, SIGNAL(acceptLocalChanges()), this, SLOT(dirtyRamAcceptLocalChanges()));
        connect(m_ramDiffWindow, SIGNAL(rejectLocalChanges()), this, SLOT(dirtyRamRejectLocalChanges()));
        m_ramDiffWindow->show();
    }
    m_ramDiffWindow->setDirtyLocation(locationid);
    //QMessageBox::information(0,"Error","Ram location dirty 0x" + QString::number(locationid,16));
}
void MainWindow::deviceRamLocationOutOfSync(unsigned short locationid)
{
    if (!m_ramDiffWindow) {
        m_ramDiffWindow = new RamDiffWindow();
        m_ramDiffWindow->setAcceptText("Write All Changes To Flash");
        m_ramDiffWindow->setRejectText("Ignore");
        connect(m_ramDiffWindow, SIGNAL(acceptLocalChanges()), this, SLOT(deviceRamAcceptChanges()));
        connect(m_ramDiffWindow, SIGNAL(rejectLocalChanges()), this, SLOT(deviceRamIgnoreChanges()));
        m_ramDiffWindow->show();
    }
    m_ramDiffWindow->setDirtyLocation(locationid);
    //QMessageBox::information(0,"Error","Ram location dirty 0x" + QString::number(locationid,16));
}
void MainWindow::deviceRamAcceptChanges()
{
    if (m_ramDiffWindow) {
        m_ramDiffWindow->close();
    }
    QMessageBox::information(0, "Error", "Write All Changes is not enabled. Please manually write all locations");
    m_statusView->setEmsMemoryDirty();
}
void MainWindow::deviceRamIgnoreChanges()
{
    if (m_ramDiffWindow) {
        m_ramDiffWindow->close();
    }
    m_statusView->setEmsMemoryDirty();
}

void MainWindow::dirtyRamAcceptLocalChanges()
{
    m_emsComms->acceptLocalChanges();
}
void MainWindow::dirtyRamRejectLocalChanges()
{
    m_emsComms->rejectLocalChanges();
}

void MainWindow::flashLocationDirty(unsigned short locationid)
{
    QMessageBox::information(0, "Error", "Flash location dirty 0x" + QString::number(locationid, 16));
}
void MainWindow::closeEvent(QCloseEvent *evt)
{
    Q_UNUSED(evt);
    //Save the window state.
    /*	QMap<unsigned short,QWidget*> m_rawDataView;
    QMap<unsigned short,ConfigView*> m_configDataView;
    QMdiSubWindow *tablesMdiWindow;
    QMdiSubWindow *firmwareMetaMdiWindow;
    QMdiSubWindow *interrogateProgressMdiWindow;
    QMdiSubWindow *emsMdiWindow;
    QMdiSubWindow *flagsMdiWindow;
    QMdiSubWindow *gaugesMdiWindow;
    QMdiSubWindow *packetStatusMdiWindow;
    QMdiSubWindow *aboutMdiWindow;
    QMdiSubWindow *emsStatusMdiWindow;

    ParameterView *parameterView;
    QMdiSubWindow *parameterMdiWindow;*/
    QSettings windowsettings(m_settingsFile, QSettings::IniFormat);
    windowsettings.beginWriteArray("rawwindows");
    int val = 0;
    for (QMap<unsigned short, QWidget *>::const_iterator i = m_rawDataView.constBegin(); i != m_rawDataView.constEnd(); i++) {
        windowsettings.setArrayIndex(val++);
        windowsettings.setValue("window", i.key());
        QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow *>(i.value()->parent());
        windowsettings.setValue("location", subwin->saveGeometry());
        windowsettings.setValue("hidden", subwin->isHidden());
        windowsettings.setValue("type", i.value()->metaObject()->className());
    }

    //tablesMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "tablesMdiWindow");
    if (m_tablesMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_tablesMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_tablesMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //firmwareMetaMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "firmwareMetaMdiWindow");
    if (m_firmwareMetaMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_firmwareMetaMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_firmwareMetaMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //interrogateProgressMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "interrogateProgressMdiWindow");
    if (m_interrogateProgressMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_interrogateProgressMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_interrogateProgressMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //emsMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "emsMdiWindow");
    if (m_emsMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_emsMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_emsMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //flagsMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "flagsMdiWindow");
    if (m_flagsMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_flagsMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_flagsMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //gaugesMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "gaugesMdiWindow");
    if (m_gaugesMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_gaugesMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_gaugesMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //packetStatusMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "packetStatusMdiWindow");
    if (m_packetStatusMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_packetStatusMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_packetStatusMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //aboutMdiWindow
    windowsettings.setArrayIndex(val++);
    windowsettings.setValue("type", "aboutMdiWindow");
    if (m_aboutMdiWindow) {
        windowsettings.setValue("enabled", true);
        windowsettings.setValue("location", m_aboutMdiWindow->saveGeometry());
        windowsettings.setValue("hidden", m_aboutMdiWindow->isHidden());
    } else {
        windowsettings.setValue("enabled", false);
    }

    //emsStatusMdiWindow
    //windowsettings.setArrayIndex(val++);
    //windowsettings.setValue("location",emsStatusMdiWindow->saveGeometry());
    ////windowsettings.setValue("hidden",emsStatusMdiWindow->isHidden());
    //windowsettings.setValue("type","emsStatusMdiWindow");

    windowsettings.endArray();
    windowsettings.beginGroup("general");
    windowsettings.setValue("location", this->saveGeometry());
    windowsettings.setValue("isMaximized", this->isMaximized());
    windowsettings.endGroup();
    windowsettings.sync();
    if (m_emsComms) {
        QString compat = m_emsComms->getPluginCompat();
        windowsettings.setValue("plugincompat", compat);
        windowsettings.sync();
    }
}
