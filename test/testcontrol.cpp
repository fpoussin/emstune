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
	QString json = "";
	json += "{ \"descriptor\" : [";

	json += "{";
	json += "\"start\" : 0,";
	json += "\"size\" : 2,";
	json += "\"is_signed\" : true,";
	json += "\"name\" : \"IAT\",";
	json += "\"description\" : \"Intake Air Temperature\",";
	json += "\"multiplier\" : \"0.01\",";
	json += "\"adder\" : \"-273.15\"";
	json += "},";

	json += "{";
	json += "\"start\" : 2,";
	json += "\"size\" : 2,";
	json += "\"is_signed\" : true,";
	json += "\"name\" : \"CHT\",";
	json += "\"description\" : \"Coolant/Head Temperature\",";
	json += "\"multiplier\" : \"0.01\",";
	json += "\"adder\" : \"-273.15\"";
	json += "},";

	json += "]}";
	datalogDescriptor = json;
	m_datalogDescriptorCount = 4;


	m_3dlocationIdList.append(0x000);
	m_3dlocationIdList.append(0x001);
	m_3dlocationIdList.append(0x002);
	m_3dlocationIdList.append(0x005);

	m_2dlocationIdList.append(0x003);
	m_2dlocationIdList.append(0x004);
	m_2dlocationIdList.append(0x006);
}
void TestControl::start()
{

	m_testList.append(QString(SLOT(TEST_table3ddata_setData())));
	m_testList.append(QString(SLOT(TEST_table2ddata_setData())));
	m_testList.append(QString(SLOT(reportResults())));

	m_comms = new FreeEmsComms();
	connect(m_comms,SIGNAL(connected()),this,SLOT(connected()),Qt::QueuedConnection);
	m_comms->passLogger(&QsLogging::Logger::instance());
	m_comms->connectSerial("COMTEST",115200);
	connect(m_comms,SIGNAL(interrogationComplete()),this,SLOT(TEST_interrogationComplete()));
	//connect(m_comms,SIGNAL(datalogDescriptor(QString)),this,SLOT(TEST_datalogDescriptor(QString)));

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
void TestControl::getDatalogDescriptor()
{
	QTimer::singleShot(250,this,SLOT(sendDatalogDescriptor()));
}
void TestControl::sendDatalogDescriptor()
{
	if (m_datalogDescriptorCount == 0)
	{
		m_packetDecoder->datalogDescriptor(datalogDescriptor);
		m_packetDecoder->completePacketAcked(GET_DATALOG_DESCRIPTOR+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
		return;
	}
	m_datalogDescriptorCount--;
	m_packetDecoder->partialPacketAcked(GET_DATALOG_DESCRIPTOR+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
	QTimer::singleShot(250,this,SLOT(sendDatalogDescriptor()));
}
void TestControl::getLocationIdList()
{
	QTimer::singleShot(250,this,SLOT(sendLocationIdList()));
}

void TestControl::sendLocationIdList()
{
	QList<unsigned short> combinedlist;
	for (int i=0;i<m_2dlocationIdList.size();i++)
	{
		combinedlist.append(m_2dlocationIdList.at(i));
	}
	for (int i=0;i<m_3dlocationIdList.size();i++)
	{
		combinedlist.append(m_3dlocationIdList.at(i));
	}

	m_packetDecoder->locationIdList(combinedlist);
	m_packetDecoder->packetAcked(GET_LOCATION_ID_LIST+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getLocationIdInfo(unsigned short locid)
{
	m_locationIdInfoReq = locid;
	QTimer::singleShot(250,this,SLOT(sendLocationIdInfo()));
}
void TestControl::sendLocationIdInfo()
{
	//m_locationIdInfoReq
	MemoryLocationInfo info;
	info.ramaddress = m_locationIdInfoReq * 100;
	info.locationid = m_locationIdInfoReq;
	info.hasParent = false;
	info.isRam = true;
	info.isFlash = false;
	info.size = 1024;
	if (m_2dlocationIdList.contains(m_locationIdInfoReq))
	{
		info.type = DATA_TABLE_3D;
	}
	else if (m_3dlocationIdList.contains(m_locationIdInfoReq))
	{
		info.type = DATA_TABLE_3D;
	}
	else
	{
		info.type == DATA_UNDEFINED;
	}
	m_packetDecoder->locationIdInfo(info);
	m_packetDecoder->packetAcked(GET_LOCATION_ID_INFO+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
}
void TestControl::getBlockInRam(unsigned short locid,unsigned short offset,unsigned short size)
{
	m_locationIdInfoReq = locid;
	m_locationIdOffset = offset;
	m_locationIdSize = size;
	QTimer::singleShot(250,this,SLOT(sendBlockInRam()));
}
void TestControl::sendBlockInRam()
{
	QByteArray blockinram;

	//16x16 table
	blockinram.append((char)0x00);
	blockinram.append((char)0x10);
	blockinram.append((char)0x00);
	blockinram.append((char)0x10);
	//XAxis
	for (int i=0;i<16;i++)
	{
		blockinram.append((char)((i * 100) >> 8));
		blockinram.append((char)((i * 100)));
	}
	//Buffer
	while (blockinram.size() < 58)
	{
		blockinram.append((char)0xFF);
	}
	//YAxis
	for (int i=0;i<16;i++)
	{
		blockinram.append((char)((i * 200) >> 8));
		blockinram.append((char)((i * 200)));
	}
	//Buffer
	while (blockinram.size() < 100)
	{
		blockinram.append((char)0xFF);
	}
	//Values
	for (int i=0;i<16*16*2;i++)
	{
		blockinram.append((char)i);
	}
	while (blockinram.size() < 1024)
	{
		blockinram.append((char)0xFF);
	}

	m_packetDecoder->ramBlockUpdatePacket(QByteArray().append((char)0x01).append((char)0x00),blockinram);
	m_packetDecoder->packetAcked(RETRIEVE_BLOCK_IN_RAM+1,QByteArray().append((char)0x01).append((char)0x00),QByteArray().append((char)0x01).append((char)0x00));
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
void TestControl::TEST_interrogationComplete()
{
	//Verify data is as expected
	//Testing
	nextTest();

}
void TestControl::nextTest()
{
	if (m_testList.size() > 0)
	{
		QTimer::singleShot(250,this,m_testList.at(0).toStdString().c_str());
		m_testList.removeAt(0);
	}
}

void TestControl::TEST_table3ddata_setData()
{
	//void FETable3DData::setData(unsigned short locationid,bool isflashonly, QByteArray data)
	//Test for every 3D table location
	for (int i=0;i<m_3dlocationIdList.size();i++)
	{
		Table3DData *data = m_comms->get3DTableData(m_3dlocationIdList.at(i));
		QList<double> xaxis = data->xAxis();
		int endvalue = 0;
		bool failed = false;
		for (int x=0;x<xaxis.size();x++)
		{
			int value = (int)xaxis.at(x);
			if (value != endvalue)
			{
				failed = true;
				break;
			}
			endvalue += 100;
		}
		m_testResults.append(QPair<QString,bool>("3D Tables:X Axis Encoding",failed));

		QList<double> yaxis = data->yAxis();
		endvalue = 0;
		failed = false;
		for (int y=0;y<yaxis.size();y++)
		{
			int value = (int)yaxis.at(y);
			if (value != endvalue)
			{
				failed = true;
			}
			endvalue += 200;
		}
		m_testResults.append(QPair<QString,bool>("3D Tables:Y Axis Encoding",failed));
	}
	nextTest();
}
void TestControl::TEST_table2ddata_setData()
{

}

void TestControl::reportResults()
{
	QLOG_DEBUG() << "TEST\t\t\tRESULT";
	for (int i=0;i<m_testResults.size();i++)
	{
		QLOG_DEBUG() << m_testResults.at(i).first + "\t\t\t" + (m_testResults.at(i).second ? "FAIL" : "PASS");
	}
}
