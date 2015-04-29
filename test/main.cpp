/************************************************************************************
 * EMStudio - Open Source ECU tuning software                                       *
 * Copyright (C) 2015  Michael Carpenter (malcom2073@gmail.com)                     *
 *                                                                                  *
 * This file is a part of EMStudio                                                  *
 *                                                                                  *
 * EMStudio is free software; you can redistribute it and/or                        *
 * modify it under the terms of the GNU General Public                              *
 * License as published by the Free Software Foundation, version                    *
 * 2 of the License.                                                                *
 *                                                                                  *
 * EMStudio is distributed in the hope that it will be useful,                      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 * General Public License for more details.                                         *
 *                                                                                  *
 * You should have received a copy of the GNU General Public                        *
 * License along with this program;  If not, see <http://www.gnu.org/licenses/>.    *
 ************************************************************************************/

#include <QCoreApplication>
#include "testcontrol.h"
#include "QsLog.h"
TestControl *m_testControl;
PacketDecoder *m_packetDecoder;
int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QsLogging::Logger& logger = QsLogging::Logger::instance();
	logger.setLoggingLevel(QsLogging::TraceLevel);
//	const QString sLogPath(QDir(appDataDir + "/EMStudio/applogs").filePath("log.txt"));

//	QsLogging::DestinationPtr fileDestination(QsLogging::DestinationFactory::MakeFileDestination(sLogPath, true, 0, 100));
	QsLogging::DestinationPtr debugDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
	logger.addDestination(debugDestination);
//	logger.addDestination(fileDestination);



	m_testControl = new TestControl();
	QTimer::singleShot(10,m_testControl,SLOT(start()));
	return a.exec();
}
