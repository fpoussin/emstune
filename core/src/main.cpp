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

#include <cassert>
#include <QApplication>
#include "mainwindow.h"
#include <QString>
#include "QsLog.h"
#include "pluginmanager.h"
#include <QCommandLineParser>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCommandLineParser parser;
    const QProcessEnvironment env(QProcessEnvironment::systemEnvironment());

    parser.setApplicationDescription("EMSTune");
    parser.addHelpOption();
    parser.addVersionOption();

    //Init the logger
    QsLogging::Logger &logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);
#ifdef Q_OS_WIN
    QString appDataDir(env.value("USERPROFILE").replace("\\", "/"));
#else
    QString appDataDir(env.value("HOME"));
#endif
    QDir appDir(appDataDir);
    if (appDir.exists()) {
        if (!appDir.cd("EMStudio")) {
            appDir.mkdir("EMStudio");
            appDir.cd("EMStudio");
        }
        if (!appDir.cd("applogs")) {
            appDir.mkdir("applogs");
        }
    }
    const QString sLogPath(QDir(appDataDir + "/EMStudio/applogs").filePath("log.txt"));

    QsLogging::DestinationPtr fileDestination(QsLogging::DestinationFactory::MakeFileDestination(sLogPath, true, 0, 100));
    QsLogging::DestinationPtr debugDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
    logger.addDestination(debugDestination);
    logger.addDestination(fileDestination);

    QString port = "";
    QString plugin = "";
    bool autoconnect = true;

    QCommandLineOption devOption(QStringList() << "d"
                                               << "dev",
                                 "Serial device", "file");

    QCommandLineOption pluginOption(QStringList() << "p"
                                                  << "plugin",
                                    "Extra plugin", "file");

    QCommandLineOption autoconnectOption(QStringList() << "a"
                                                       << "autoconnect",
                                         "Autoconnect", "file");

    parser.addOptions({ devOption, pluginOption, autoconnectOption });

    parser.process(a);
    autoconnect = parser.isSet(autoconnectOption);
    if (parser.isSet(devOption)) {
        port = parser.value(devOption);
    }
    if (parser.isSet(pluginOption)) {
        plugin = parser.value(pluginOption);
    }

    MainWindow *w = new MainWindow();
    if (port != "") {
        w->setDevice(port);
    }
    if (plugin == "") {
        //A specific plugin is specified, override the plugin manager's choice.
    }
    w->setPlugin(plugin);
    if (autoconnect) {
        w->connectToEms();
    }
    w->show();
    PluginManager *manager = new PluginManager();
    manager->show();
    w->connect(manager, SIGNAL(fileSelected(QString)), w, SLOT(setPlugin(QString)));
    return a.exec();
}
