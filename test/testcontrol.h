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

#ifndef TESTCONTROL_H
#define TESTCONTROL_H
#include <QString>
#include <QObject>
#include "../plugins/libreems/freeemscomms.h"
class TestControl : public QObject
{
	Q_OBJECT
public:
	TestControl();
	void setMessage(QString message);
	void firmwareMessage();
	void getInterfaceVersion();
	void getCompilerVersion();
	void getDecoderName();
	void getFirmwareBuildDate();
	void getMaxPacketSize();
	void getOperatingSystem();
	void getDatalogDescriptor();
	void getLocationIdList();
	void getLocationIdInfo(unsigned short locid);
	void getBlockInRam(unsigned short locid,unsigned short offset,unsigned short size);
private:
	FreeEmsComms *m_comms;
	QList<QString> jsonList;
	int m_datalogDescriptorCount;
	QString datalogDescriptor;
	QList<unsigned short> m_locationIdList;
	unsigned short m_locationIdInfoReq;
	unsigned short m_locationIdOffset;
	unsigned short m_locationIdSize;
private slots:
	void sendBlockInRam();
	void sendLocationIdInfo();
	void sendLocationIdList();
	void sendDatalogDescriptor();
	void sendOperatingSystem();
	void sendMaxPacketSize();
	void sendFirmwareBuildDate();
	void sendDecoderName();
	void firmwareTimerTick();
	void sendInterfaceVersion();
	void sendCompilerVersion();
	void start();
	void connected();
};

#endif // TESTCONTROL_H
