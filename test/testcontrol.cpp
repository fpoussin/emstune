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

#include "testcontrol.h"
#include "QsLog.h"
extern PacketDecoder *m_packetDecoder;
TestControl::TestControl()
{
	QLOG_DEBUG() << "Started test";

}
void TestControl::start()
{
	m_comms = new FreeEmsComms();
	connect(m_comms,SIGNAL(connected()),this,SLOT(connected()),Qt::QueuedConnection);
	m_comms->passLogger(&QsLogging::Logger::instance());
	m_comms->connectSerial("COMTEST",115200);

//	connect(m_comms,SIGNAL())
}

void TestControl::setMessage(QString message)
{

}
void TestControl::connected()
{
	m_comms->startInterrogation();
}

void TestControl::getInterfaceVersion()
{
	QTimer::singleShot(250,this,SLOT(sendInterfaceVersion()));
}
void TestControl::sendInterfaceVersion()
{
	m_packetDecoder->interfaceVersion("TEST INTERFACE VERSION");
	m_packetDecoder->packetAcked(GET_INTERFACE_VERSION+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getCompilerVersion()
{
	QTimer::singleShot(250,this,SLOT(sendCompilerVersion()));
}
void TestControl::sendCompilerVersion()
{
	m_packetDecoder->compilerVersion("TEST COMPILER VERSION");
	m_packetDecoder->packetAcked(GET_COMPILER_VERSION+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getDecoderName()
{
	QTimer::singleShot(250,this,SLOT(sendDecoderName()));
}
void TestControl::sendDecoderName()
{
	m_packetDecoder->decoderName("TEST DECODER NAME");
	m_packetDecoder->packetAcked(GET_DECODER_NAME+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getFirmwareBuildDate()
{
	QTimer::singleShot(250,this,SLOT(sendFirmwareBuildDate()));
}
void TestControl::sendFirmwareBuildDate()
{
	m_packetDecoder->decoderName("TEST FIRMWARE BUILD DATE");
	m_packetDecoder->packetAcked(GET_FIRMWARE_BUILD_DATE+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getMaxPacketSize()
{
	QTimer::singleShot(250,this,SLOT(sendMaxPacketSize()));
}

void TestControl::sendMaxPacketSize()
{
	//m_packetDecoder->
	m_packetDecoder->packetAcked(GET_MAX_PACKET_SIZE+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getOperatingSystem()
{
	QTimer::singleShot(250,this,SLOT(sendOperatingSystem()));
}
void TestControl::sendOperatingSystem()
{
	m_packetDecoder->operatingSystem("TEST OPERATING SYSTEM");
	m_packetDecoder->packetAcked(GET_OPERATING_SYSTEM+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}

void TestControl::firmwareMessage()
{
	//We got a firmware request message
	QTimer::singleShot(250,this,SLOT(firmwareTimerTick()));
}

void TestControl::firmwareTimerTick()
{
	m_packetDecoder->firmwareVersion("TEST VERSION");
	m_packetDecoder->packetAcked(GET_FIRMWARE_VERSION+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
