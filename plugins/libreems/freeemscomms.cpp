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

#include "freeemscomms.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QXmlStreamReader>
#include "fetable2ddata.h"
#include "fetable3ddata.h"
#include "feconfigdata.h"
#include "QsLog.h"
#include "paths.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
FreeEmsComms::FreeEmsComms(QObject *parent) : EmsComms(parent)
{
	qRegisterMetaType<QList<unsigned short> >("QList<unsigned short>");
	qRegisterMetaType<QList<LocationIdFlags> >("QList<LocationIdFlags>");
	qRegisterMetaType<SerialPortStatus>("SerialPortStatus");
	qRegisterMetaType<ConfigBlock>("ConfigBlock");
	qRegisterMetaType<QMap<QString,QString> >("QMap<QString,QString>");
	m_state = 0; //0 is not connected.
	m_lastMessageSentTime = 0;
	m_firstPacketValid = false;
	//serialPort = new SerialPort(this);
	//connect(serialPort,SIGNAL(dataWritten(QByteArray)),this,SLOT(dataLogWrite(QByteArray)));
	m_isConnected = false;
	m_serialPortMutex.lock();

	m_packetProcessingThread = new QThread(this);
	m_packetProcessingThread->start();

	dataPacketDecoder = new FEDataPacketDecoder();
	connect(dataPacketDecoder,SIGNAL(payloadDecoded(QVariantMap)),this,SIGNAL(dataLogPayloadDecoded(QVariantMap)));
	connect(dataPacketDecoder,SIGNAL(resetDetected(int)),this,SIGNAL(resetDetected(int)));
	m_metaDataParser = new FEMemoryMetaData();

	//m_metaDataParser->loadMetaDataFromFile(Paths::getDefaultsDir() + "/libreems.config.json");
	m_metaDataParser->loadMetaDataFromFile("libreems.config.json");


	m_packetDecoder = new PacketDecoder();
	m_packetDecoder->moveToThread(m_packetProcessingThread);


	m_protocolDecoder = new ProtocolDecoder();
	m_protocolDecoder->moveToThread(m_packetProcessingThread);
	connect(m_protocolDecoder,SIGNAL(newPacket(QByteArray)),m_packetDecoder,SLOT(parseBufferToPacket(QByteArray)));



	connect(m_packetDecoder,SIGNAL(locationIdInfo(MemoryLocationInfo)),this,SLOT(locationIdInfoRec(MemoryLocationInfo)));
	connect(m_packetDecoder,SIGNAL(packetAcked(unsigned short,QByteArray,QByteArray)),this,SLOT(packetAckedRec(unsigned short,QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(partialPacketAcked(unsigned short,QByteArray,QByteArray)),this,SLOT(partialPacketAckedRec(unsigned short,QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(completePacketAcked(unsigned short,QByteArray,QByteArray)),this,SLOT(completePacketAckedRec(unsigned short,QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(packetNaked(unsigned short,QByteArray,QByteArray,unsigned short)),this,SLOT(packetNakedRec(unsigned short,QByteArray,QByteArray,unsigned short)));
	connect(m_packetDecoder,SIGNAL(locationIdList(QList<unsigned short>)),this,SLOT(locationIdListRec(QList<unsigned short>)));
	connect(m_packetDecoder,SIGNAL(ramBlockUpdatePacket(QByteArray,QByteArray)),this,SLOT(ramBlockUpdateRec(QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(flashBlockUpdatePacket(QByteArray,QByteArray)),this,SLOT(flashBlockUpdateRec(QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(dataLogPayloadReceived(QByteArray,QByteArray)),this,SIGNAL(dataLogPayloadReceived(QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(dataLogPayloadReceived(QByteArray,QByteArray)),dataPacketDecoder,SLOT(decodePayloadPacket(QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(dataLogPayloadReceived(QByteArray,QByteArray)),this,SLOT(dataLogPayloadReceivedRec(QByteArray,QByteArray)));
	connect(m_packetDecoder,SIGNAL(compilerVersion(QString)),this,SLOT(compilerVersion(QString)));
	connect(m_packetDecoder,SIGNAL(decoderName(QString)),this,SLOT(decoderName(QString)));
	connect(m_packetDecoder,SIGNAL(firmwareBuild(QString)),this,SLOT(firmwareBuild(QString)));
	connect(m_packetDecoder,SIGNAL(firmwareVersion(QString)),this,SLOT(firmwareVersion(QString)));
	connect(m_packetDecoder,SIGNAL(interfaceVersion(QString)),this,SLOT(interfaceVersion(QString)));
	connect(m_packetDecoder,SIGNAL(operatingSystem(QString)),this,SLOT(operatingSystem(QString)));
	connect(m_packetDecoder,SIGNAL(builtByName(QString)),this,SLOT(builtByName(QString)));
	connect(m_packetDecoder,SIGNAL(supportEmail(QString)),this,SLOT(supportEmail(QString)));
	connect(m_packetDecoder,SIGNAL(benchTestReply(unsigned short,unsigned char)),this,SIGNAL(benchTestReply(unsigned short,unsigned char)));
	connect(m_packetDecoder,SIGNAL(datalogDescriptor(QString)),this,SIGNAL(datalogDescriptor(QString)));
	connect(m_packetDecoder,SIGNAL(datalogDescriptor(QString)),this,SLOT(saveDatalogDescriptor(QString)));
	connect(m_packetDecoder,SIGNAL(fieldDescriptor(QString)),this,SLOT(fieldDescriptor(QString)));
	connect(m_packetDecoder,SIGNAL(tableDescriptor(QString)),this,SLOT(tableDescriptor(QString)));
	connect(m_packetDecoder,SIGNAL(firmwareDebug(QString)),this,SLOT(firmwareDebug(QString)));

	connect(m_packetDecoder,SIGNAL(datalogDescriptor(QString)),dataPacketDecoder,SLOT(datalogDescriptor(QString)));

	m_lastdatalogTimer = new QTimer(this);
	connect(m_lastdatalogTimer,SIGNAL(timeout()),this,SLOT(datalogTimerTimeout()));
	m_lastdatalogTimer->start(500); //Every half second, check to see if we've timed out on datalogs.

	m_waitingForResponse = false;
	m_logsEnabled = false;
	m_lastDatalogUpdateEnabled = false;
	m_logInFile=0;
	m_logOutFile=0;
	m_logInOutFile=0;
	m_debugLogsEnabled = false;
	m_waitingForRamWrite=false;
	m_waitingForFlashWrite=false;
	m_sequenceNumber = 1;
	m_blockFlagList.append(BLOCK_HAS_PARENT);
	m_blockFlagList.append(BLOCK_IS_RAM);
	m_blockFlagList.append(BLOCK_IS_FLASH);
	m_blockFlagList.append(BLOCK_IS_INDEXABLE);
	m_blockFlagList.append(BLOCK_IS_READ_ONLY);
	m_blockFlagList.append(BLOCK_GETS_VERIFIED);
	m_blockFlagList.append(BLOCK_FOR_BACKUP_RESTORE);
	m_blockFlagList.append(BLOCK_SPARE_7);
	m_blockFlagList.append(BLOCK_SPARE_8);
	m_blockFlagList.append(BLOCK_SPARE_9);
	m_blockFlagList.append(BLOCK_SPARE_10);
	m_blockFlagList.append(BLOCK_IS_2D_SIGNED_TABLE);
	m_blockFlagList.append(BLOCK_IS_2D_TABLE);
	m_blockFlagList.append(BLOCK_IS_MAIN_TABLE);
	m_blockFlagList.append(BLOCK_IS_LOOKUP_DATA);
	m_blockFlagList.append(BLOCK_IS_CONFIGURATION);


	m_blockFlagToNameMap[BLOCK_HAS_PARENT] = "Parent";
	m_blockFlagToNameMap[BLOCK_IS_RAM] = "Is Ram";
	m_blockFlagToNameMap[BLOCK_IS_FLASH] = "Is Flash";
	m_blockFlagToNameMap[BLOCK_IS_INDEXABLE] = "Is Indexable";
	m_blockFlagToNameMap[BLOCK_IS_READ_ONLY] = "Is Read Only";
	m_blockFlagToNameMap[BLOCK_FOR_BACKUP_RESTORE] = "For Backup";
	m_blockFlagToNameMap[BLOCK_GETS_VERIFIED] = "Is Verified";
	m_blockFlagToNameMap[BLOCK_IS_2D_TABLE] = "2D Table";
	m_blockFlagToNameMap[BLOCK_IS_2D_SIGNED_TABLE] = "2D Signed Table";
	m_blockFlagToNameMap[BLOCK_IS_MAIN_TABLE] = "3D Table";
	m_blockFlagToNameMap[BLOCK_IS_LOOKUP_DATA] = "Lookup Table";
	m_blockFlagToNameMap[BLOCK_IS_CONFIGURATION] = "Configuration";
	m_interrogateInProgress = false;
	m_interogateComplete = false;
	m_interrogateIdListComplete = false;
	m_interrogateIdInfoComplete = false;
	m_interrogateTotalCount=0;
	emsData.setMetaData(m_metaDataParser);
	connect(&emsData,SIGNAL(updateRequired(unsigned short)),this,SIGNAL(deviceDataUpdated(unsigned short)),Qt::DirectConnection);
	connect(&emsData,SIGNAL(ramBlockUpdateRequest(unsigned short,unsigned short,unsigned short,QByteArray)),this,SLOT(updateBlockInRam(unsigned short,unsigned short,unsigned short,QByteArray)),Qt::DirectConnection);
	connect(&emsData,SIGNAL(flashBlockUpdateRequest(unsigned short,unsigned short,unsigned short,QByteArray)),this,SLOT(updateBlockInFlash(unsigned short,unsigned short,unsigned short,QByteArray)),Qt::DirectConnection);
	connect(&emsData,SIGNAL(updateRequired(unsigned short)),this,SLOT(locationIdUpdate(unsigned short)),Qt::DirectConnection);
	connect(&emsData,SIGNAL(configRecieved(ConfigBlock,QVariant)),this,SIGNAL(configRecieved(ConfigBlock,QVariant)),Qt::DirectConnection);
	connect(&emsData,SIGNAL(localRamLocationDirty(unsigned short)),this,SLOT(ramLocationMarkedDirty(unsigned short)),Qt::DirectConnection);
	connect(&emsData,SIGNAL(localFlashLocationDirty(unsigned short)),this,SLOT(flashLocationMarkedDirty(unsigned short)),Qt::DirectConnection);


	QFile dialogFile("menuconfig.json");
	if (!dialogFile.open(QIODevice::ReadOnly))
	{
		return;
	}
	QByteArray dialogfiledata = dialogFile.readAll();
	dialogFile.close();

	QJsonDocument document = QJsonDocument::fromJson(dialogfiledata);
	QJsonObject topmap = document.object();

	QJsonArray dialogslist = topmap.value("dialogs").toArray();
	MenuSetup menu;
	for (int i=0;i<dialogslist.size();i++)
	{
		QJsonObject dialogitemmap = dialogslist.at(i).toObject();

		QJsonArray fieldlist = dialogitemmap.value("fieldlist").toArray();
		DialogItem item;
		item.variable = dialogitemmap.value("variable").toString();
		item.title = dialogitemmap.value("title").toString();
		for (int j=0;j<fieldlist.size();j++)
		{
			QJsonObject fieldmap = fieldlist.at(j).toObject();
			DialogField field;
			field.title = fieldmap.value("title").toString();
			field.variable = fieldmap.value("variable").toString();
			field.condition = fieldmap.value("condition").toString();
			item.fieldList.append(field);
		}
		menu.dialoglist.append(item);
	}

	QJsonArray menulist = topmap.value("menu").toArray();
	for (int i=0;i<menulist.size();i++)
	{
		QJsonObject menuitemmap = menulist.at(i).toObject();
		MenuItem menuitem;
		menuitem.title = menuitemmap.value("title").toString();
		QJsonArray submenuitemlist = menuitemmap.value("subitems").toArray();
		for (int j=0;j<submenuitemlist.size();j++)
		{
			QJsonObject submenuitemmap = submenuitemlist.at(j).toObject();
			SubMenuItem submenuitem;
			submenuitem.title = submenuitemmap.value("title").toString();
			submenuitem.variable = submenuitemmap.value("variable").toString();
			menuitem.subMenuList.append(submenuitem);
		}
		menu.menulist.append(menuitem);
	}

	QJsonArray configlist = topmap.value("config").toArray();
	QMap<QString,QList<ConfigBlock> > configmap;
	for (int i=0;i<configlist.size();i++)
	{
		QJsonObject configitemmap = configlist.at(i).toObject();
		FEConfigData *block = new FEConfigData();
		//connect(data,SIGNAL(saveSingleDataToFlash(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(flashBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
		connect(block,SIGNAL(saveSingleDataToFlash(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(flashBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
		block->setName(configitemmap.value("name").toString());
		block->setType(configitemmap.value("type").toString());
		block->setElementSize(configitemmap.value("sizeofelement").toInt());
		block->setSize(configitemmap.value("size").toInt());
		block->setOffset(configitemmap.value("offset").toInt());
		QList<QPair<QString,double> > parsedcalclist;
		QJsonArray calclist = configitemmap.value("calc").toArray();
		for (int j=0;j<calclist.size();j++)
		{
			QJsonObject calcitemmap = calclist.at(j).toObject();
			parsedcalclist.append(QPair<QString,double>(calcitemmap.value("type").toString(),calcitemmap.value("value").toDouble()));
		}
		block->setCalc(parsedcalclist);

		//configitemmap["calc"];
		block->setSizeOverride(configitemmap["sizeoverride"].toString());
		bool ok = false;
		block->setLocationId(configitemmap["locationid"].toString().toInt(&ok,16));
		QString locid = configitemmap["locationid"].toString();

		unsigned short locidint = locid.mid(2).toInt(&ok,16);

		if (!ok)
		{
		    //Invalid location id conversion, bad JSON file?
		}
		else
		{
			if (!m_locIdToConfigListMap.contains(locidint))
			{
				m_locIdToConfigListMap[locidint] = QList<FEConfigData*>();
			}
			m_locIdToConfigListMap[locidint].append(block);
		}
		if (m_configMap.contains(block->name()))
		{
		    //Block already exists!
		}
		else
		{
		    m_configMap[block->name()] = block;
		}
	}

    //m_metaDataParser->passConfigData(configmap);
	m_metaDataParser->setMenuMetaData(menu);
	//start();
	m_sentMessageTimeoutTimer = new QTimer(this);
	connect(m_sentMessageTimeoutTimer,SIGNAL(timeout()),this,SLOT(timeoutTimerTick()));
	m_sentMessageTimeoutTimer->start(250);


}
void FreeEmsComms::writeAllRamToRam()
{
	QList<unsigned short> ramlist = emsData.getTopLevelDeviceRamLocations();

	for (int i=0;i<ramlist.size();i++)
	{
		if (!emsData.isReadOnlyRamBlock(ramlist[i]))
		{
			updateBlockInRam(ramlist[i],0,emsData.getDeviceRamBlock(ramlist[i]).size(),emsData.getDeviceRamBlock(ramlist[i]));
		}
	}
}

void FreeEmsComms::decoderName(QString name)
{
	m_interrogationMetaDataMap["Decoder Name"] = name;
}

void FreeEmsComms::firmwareBuild(QString date)
{
	m_interrogationMetaDataMap["Firmware Build Date"] = date;
}

void FreeEmsComms::compilerVersion(QString version)
{
	m_interrogationMetaDataMap["Compiler Version"] = version;
}

void FreeEmsComms::operatingSystem(QString os)
{
	m_interrogationMetaDataMap["Operating System"] = os;
}

void FreeEmsComms::interfaceVersion(QString version)
{
	m_interrogationMetaDataMap["Interface Version"] = version;
}

void FreeEmsComms::firmwareVersion(QString version)
{
	if (m_state == 1)
	{
		//Waiting on first FW version reply packet
		//Good to go!
		m_state = 2; //2 is running, kick off the next packet send.
		m_waitingForResponse = false; //Clear this out, since we're not parsing the packet direct.
		m_lastDatalogUpdateEnabled = true;
		emit connected();
		QLOG_DEBUG() << "Connected...." << m_reqList.size() << "Items in queue";
	}
	m_interrogationMetaDataMap["Firmware Version"] = version;
}
void FreeEmsComms::builtByName(QString name)
{
	m_interrogationMetaDataMap["Built By"] = name;
}

void FreeEmsComms::supportEmail(QString email)
{
	m_interrogationMetaDataMap["Support Email"] = email;
}

void FreeEmsComms::passLogger(QsLogging::Logger *log)
{
	//Set the internal instance.
	QsLogging::Logger::instance(log);
	QLOG_DEBUG() << "Logging from the plugin!!!";
}

MemoryMetaData *FreeEmsComms::getMetaParser()
{
	return m_metaDataParser;
}

DataPacketDecoder *FreeEmsComms::getDecoder()
{
	return dataPacketDecoder;
}
Table3DData *FreeEmsComms::getNew3DTableData()
{
	return new FETable3DData();
}

Table2DData *FreeEmsComms::getNew2DTableData()
{
	return new FETable2DData(DATA_UNDEFINED);
}

FreeEmsComms::~FreeEmsComms()
{
}

void FreeEmsComms::disconnectSerial()
{
	m_lastDatalogUpdateEnabled = false;
	serialPort->closePort();
	m_state = 1;
	serialPort->wait(2000);
	serialPort->terminate();
	delete serialPort;
	serialPort = 0;
	emit disconnected();
	RequestClass req;
	req.type = SERIAL_DISCONNECT;
}
void FreeEmsComms::startInterrogation()
{
	if (!m_interrogateInProgress)
	{
		RequestClass req;
		req.type = INTERROGATE_START;
		m_interrogatePacketList.clear();
		m_interrogateInProgress = true;
		m_interrogateIdListComplete = false;
		m_interrogateIdInfoComplete = false;
		int seq = getFirmwareVersion();
		emit interrogateTaskStart("Ecu Info FW Version",seq);
		m_interrogatePacketList.append(seq);

		seq = getInterfaceVersion();
		emit interrogateTaskStart("Ecu Info Interface Version",seq);
		m_interrogatePacketList.append(seq);

		seq = getCompilerVersion();
		emit interrogateTaskStart("Ecu Info Compiler Version",seq);
		m_interrogatePacketList.append(seq);

		seq = getDecoderName();
		emit interrogateTaskStart("Ecu Info Decoder Name",seq);
		m_interrogatePacketList.append(seq);

		seq = getFirmwareBuildDate();
		emit interrogateTaskStart("Ecu Info Firmware Build Date",seq);
		m_interrogatePacketList.append(seq);

		seq = getMaxPacketSize();
		emit interrogateTaskStart("Ecu Info Max Packet Size",seq);
		m_interrogatePacketList.append(seq);

		seq = getOperatingSystem();
		emit interrogateTaskStart("Ecu Info Operating System",seq);
		m_interrogatePacketList.append(seq);

		//seq = getBuiltByName();
		//emit interrogateTaskStart("Ecu Info Built By Name",seq);
		//m_interrogatePacketList.append(seq);


		//seq = getSupportEmail();
		//emit interrogateTaskStart("Ecu Info Support Email",seq);
		//m_interrogatePacketList.append(seq);

		seq = getDatalogDescriptor();
		emit interrogateTaskStart("Datalog Descriptor request",seq);
		m_interrogatePacketList.append(seq);

		seq = getFieldDescriptor();
		emit interrogateTaskStart("Field Descriptor request",seq);
		m_interrogatePacketList.append(seq);

		seq = getTableDescriptor();
		emit interrogateTaskStart("Table Descriptor request",seq);
		m_interrogatePacketList.append(seq);



		seq = getLocationIdList(0x00,0x00);
		emit interrogateTaskStart("Ecu Info Location ID List",seq);
		m_interrogatePacketList.append(seq);

		m_interrogateTotalCount=6;

		QLOG_DEBUG() << "Interrogate in progress is now true";
	}
	else
	{
		QLOG_DEBUG() << "Interrogate in progress is somehow true!!";
	}
}

void FreeEmsComms::openLogs()
{
	QLOG_INFO() << "Open logs:" << m_logsDirectory + "/" + m_logsFilename + ".bin";
	if (!QDir(m_logsDirectory).exists())
	{
		QDir dir(QCoreApplication::instance()->applicationDirPath());
		if (!dir.mkpath(m_logsDirectory))
		{
			emit error("Unable to create log directory. Data will NOT be logged until this is fixed!");
		}
	}
	m_logInFile = new QFile(m_logsDirectory + "/" + m_logsFilename + ".bin");
	m_logInFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
	if (m_debugLogsEnabled)
	{
		m_logInOutFile = new QFile(m_logsDirectory + "/" + m_logsFilename + ".both.bin");
		m_logInOutFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
		m_logOutFile = new QFile(m_logsDirectory + "/" + m_logsFilename + ".toecu.bin");
		m_logOutFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
	}
}
int FreeEmsComms::getDatalogDescriptor()
{
	RequestClass req;
	req.type = GET_DATALOG_DESCRIPTOR;
	req.sequencenumber = m_sequenceNumber;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}

int FreeEmsComms::getFieldDescriptor()
{
	RequestClass req;
	req.type = GET_FIELD_DESCRIPTOR;
	req.sequencenumber = m_sequenceNumber;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getTableDescriptor()
{
	RequestClass req;
	req.type = RETRIEVE_JSON_TABLE_DESCRIPTOR;
	req.sequencenumber = m_sequenceNumber;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
void FreeEmsComms::connectSerial(QString port,int baud)
{
	serialPort = new SerialPort();

	//connect(serialPort,SIGNAL(packetReceived(QByteArray)),this,SLOT(parseEverything(QByteArray)));
	connect(serialPort,SIGNAL(bytesReady(QByteArray)),m_protocolDecoder,SLOT(parseBuffer(QByteArray)),Qt::QueuedConnection);
	connect(serialPort,SIGNAL(bytesReady(QByteArray)),this,SLOT(dataLogRead(QByteArray)));
	connect(serialPort,SIGNAL(connected()),this,SLOT(serialPortConnected()));
	connect(serialPort,SIGNAL(unableToConnect(QString)),this,SLOT(serialPortUnableToConnect(QString)));
	connect(serialPort,SIGNAL(error(QString)),this,SLOT(serialPortError(QString)));
	connect(serialPort,SIGNAL(disconnected()),this,SLOT(serialPortDisconnected()));
	serialPort->connectToPort(port);
}
void FreeEmsComms::serialPortConnected()
{
	m_state = 1; //1 is connected, waiting for firmware_version packet
	sendPacket(GET_FIRMWARE_VERSION);
	return;
}
void FreeEmsComms::serialPortUnableToConnect(QString errorstr)
{
	emit error(QString("Error connecting to port: ") + errorstr);
}

void FreeEmsComms::loadLog(QString filename)
{
	Q_UNUSED(filename);
}

void FreeEmsComms::playLog()
{
}
void FreeEmsComms::setLogsEnabled(bool enabled)
{
	if (m_logsEnabled && !enabled)
	{
		m_logInFile->close();
		delete m_logInFile;
		m_logInFile=0;

		if (m_debugLogsEnabled)
		{
			m_logInOutFile->close();
			delete m_logInOutFile;
			m_logInOutFile=0;

			m_logOutFile->close();
			delete m_logOutFile;
			m_logOutFile=0;
		}
	}
	else if (!m_logsEnabled && enabled)
	{
		openLogs();
	}
	m_logsEnabled = enabled;
}
void FreeEmsComms::setlogsDebugEnabled(bool enabled)
{
	if (m_logsEnabled && enabled && !m_debugLogsEnabled)
	{
		if (!QDir(m_logsDirectory).exists())
		{
			QDir dir(QCoreApplication::instance()->applicationDirPath());
			if (!dir.mkpath(m_logsDirectory))
			{
				emit error("Unable to create log directory. Data will NOT be logged until this is fixed!");
			}
		}
		m_logInOutFile = new QFile(m_logsDirectory + "/" + m_logsFilename + ".both.bin");
		m_logInOutFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
		m_logOutFile = new QFile(m_logsDirectory + "/" + m_logsFilename + ".toecu.bin");
		m_logOutFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
	}
	else if (m_logsEnabled && !enabled && m_debugLogsEnabled)
	{
		m_logInOutFile->close();
		m_logInOutFile->deleteLater();
		m_logInOutFile=0;
		m_logOutFile->close();
		m_logOutFile->deleteLater();
		m_logOutFile=0;
	}
	m_debugLogsEnabled = enabled;
}

void FreeEmsComms::setLogDirectory(QString dir)
{
	m_logsDirectory = dir;
}

void FreeEmsComms::setPort(QString portname)
{
	Q_UNUSED(portname)
	//serialPort->setPort(portname);
}

void FreeEmsComms::setBaud(int baudrate)
{
	Q_UNUSED(baudrate)
	//serialPort->setBaud(baudrate);
}
int FreeEmsComms::burnBlockFromRamToFlash(unsigned short location,unsigned short offset, unsigned short size)
{
	RequestClass req;
	req.type = BURN_BLOCK_FROM_RAM_TO_FLASH;
	req.addArg(location,sizeof(location));
	req.addArg(offset,sizeof(offset));
	req.addArg(size,sizeof(size));
	req.sentRequest = true;
	req.hasReply = true;
	req.sequencenumber = m_sequenceNumber;
	m_sequenceNumber++;

	if (size == 0)
	{
		size = emsData.getLocalRamBlock(location).size();
	}

	for (int i=emsData.getLocalRamBlockInfo(location)->ramAddress + offset;i<emsData.getLocalRamBlockInfo(location)->ramAddress+offset+size;i++)
	{
		if (m_dirtyRamAddresses.contains(i))
		{
			m_dirtyRamAddresses.removeOne(i);
		}
	}
	if (m_dirtyRamAddresses.size() == 0)
	{
		emit memoryClean();
	}
	/*if (m_dirtyLocationIds.contains(location))
	{
		m_dirtyLocationIds.removeOne(location);
		if (m_dirtyLocationIds.size() == 0)
		{
			emit memoryClean();
		}
	}*/
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::enableDatalogStream()
{
	RequestClass req;
	req.type = UPDATE_BLOCK_IN_RAM;
	req.addArg(0x9000,2);
	req.addArg(0,2);
	req.addArg(1,2);
	req.addArg(QByteArray().append((char)0x01));
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}

int FreeEmsComms::disableDatalogStream()
{
	RequestClass req;
	req.type = UPDATE_BLOCK_IN_RAM;
	req.addArg(0x9000,2);
	req.addArg(0,2);
	req.addArg(1,2);
	req.addArg(QByteArray().append((char)0x00));
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}

int FreeEmsComms::updateBlockInRam(unsigned short location,unsigned short offset, unsigned short size,QByteArray data)
{
	RequestClass req;
	req.type = UPDATE_BLOCK_IN_RAM;
	req.addArg(location,sizeof(location));
	req.addArg(offset,sizeof(offset));
	req.addArg(size,sizeof(size));
	req.addArg(data);
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);

	emsData.markLocalRamLocationDirty(location,offset,size);

	if (emsData.getLocalRamBlockInfo(location)->isFlash)
	{
		unsigned short ramaddress = emsData.getLocalRamBlockInfo(location)->ramAddress;
		for (int i=ramaddress+offset;i<ramaddress+offset+size;i++)
		{
			if (!m_dirtyRamAddresses.contains(i))
			{
				m_dirtyRamAddresses.append(i);
			}
		}
		emit memoryDirty();
	}
	return m_sequenceNumber-1;
}
int FreeEmsComms::updateBlockInFlash(unsigned short location,unsigned short offset, unsigned short size,QByteArray data)
{
	RequestClass req;
	req.type = UPDATE_BLOCK_IN_FLASH;
	req.addArg(location,sizeof(location));
	req.addArg(offset,sizeof(offset));
	req.addArg(size,sizeof(size));
	req.addArg(data);
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	m_sequenceNumber++;
	emsData.markLocalFlashLocationDirty(location,offset,size);
	sendPacket(req);
	return m_sequenceNumber-1;
}

int FreeEmsComms::getDecoderName()
{
	RequestClass req;
	req.type = GET_DECODER_NAME;
	req.sentRequest = true;
	req.hasReply = true;
	req.sequencenumber = m_sequenceNumber;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getFirmwareBuildDate()
{
	RequestClass req;
	req.type = GET_FIRMWARE_BUILD_DATE;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getCompilerVersion()
{
	RequestClass req;
	req.type = GET_COMPILER_VERSION;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getOperatingSystem()
{
	RequestClass req;
	req.type = GET_OPERATING_SYSTEM;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getSupportEmail()
{
	RequestClass req;
	req.type = GET_SUPPORT_EMAIL;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getBuiltByName()
{
	RequestClass req;
	req.type = GET_BUILT_BY_NAME;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::retrieveBlockFromFlash(unsigned short location, unsigned short offset, unsigned short size,bool mark)
{
	RequestClass req;
	req.type = RETRIEVE_BLOCK_IN_FLASH;
	req.addArg(location,sizeof(location));
	req.addArg(offset,sizeof(offset));
	req.addArg(size,sizeof(size));
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	//This gets us the range of effected values.
	//emsData.getLocalRamBlockInfo(location)->size;
	//emsData.getLocalRamBlockInfo(location)->ramAddress;

	//We mark it dirty, so it will be silently replaced.
	if (mark)
	{
		emsData.markLocalFlashLocationDirty(location,offset,size);
	}

	/*if (emsData.getLocalRamBlockInfo(location))
	{
		emsData.markLocalRamLocationClean(location,offset,size);

		for (int i=emsData.getLocalRamBlockInfo(location)->ramAddress + offset;i<emsData.getLocalRamBlockInfo(location)->ramAddress+offset+size;i++)
		{
			if (m_dirtyRamAddresses.contains(i))
			{
				m_dirtyRamAddresses.removeOne(i);
			}
		}
		if (m_dirtyRamAddresses.size() == 0)
		{
			emit memoryClean();
		}
	}*/
	return m_sequenceNumber-1;
}
int FreeEmsComms::retrieveBlockFromRam(unsigned short location, unsigned short offset, unsigned short size,bool mark)
{
	RequestClass req;
	req.type = RETRIEVE_BLOCK_IN_RAM;
	req.addArg(location,sizeof(location));
	req.addArg(offset,sizeof(offset));
	req.addArg(size,sizeof(size));
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	if (mark)
	{
		emsData.markLocalRamLocationDirty(location,offset,size);
	}
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getInterfaceVersion()
{
	RequestClass req;
	req.type = GET_INTERFACE_VERSION;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getFirmwareVersion()
{
	RequestClass req;
	req.type = GET_FIRMWARE_VERSION;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::getMaxPacketSize()
{
	RequestClass req;
	req.type = GET_MAX_PACKET_SIZE;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::echoPacket(QByteArray packet)
{
	RequestClass req;
	req.type = ECHO_PACKET;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	req.addArg(packet);
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}
int FreeEmsComms::startBenchTest(unsigned char eventspercycle,unsigned short numcycles,unsigned short ticksperevent,QVariantList pineventmask,QVariantList pinmode)
{
	RequestClass req;
	req.type = BENCHTEST;
	req.addArg(0x01,sizeof(char));
	req.addArg(eventspercycle,sizeof(eventspercycle));
	req.addArg(numcycles,sizeof(numcycles));
	req.addArg(ticksperevent,sizeof(ticksperevent));
	for (int i=0;i<pineventmask.size();i++)
	{
		if ((pineventmask[i].toInt() > 255) || (pineventmask[i].toInt() < 0))
		{
			return -1;
		}
		req.addArg((unsigned char)pineventmask[i].toInt(),1);
	}
	for (int i=0;i<pinmode.size();i++)
	{
		if ((pinmode[i].toInt() < 0) || (pinmode[i].toInt() > 65535))
		{
			return -1;
		}
		req.addArg((unsigned short)pinmode[i].toInt(),2);
	}
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	m_sequenceNumber++;
	return m_sequenceNumber-1;
}
int FreeEmsComms::stopBenchTest()
{
	RequestClass req;
	req.type = BENCHTEST;
	req.addArg(0x00,sizeof(char));
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	m_sequenceNumber++;
	return m_sequenceNumber-1;
}
int FreeEmsComms::bumpBenchTest(unsigned char cyclenum)
{
	RequestClass req;
	req.type = BENCHTEST;
	req.addArg(0x02,sizeof(char));
	req.addArg(cyclenum,sizeof(cyclenum));
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	m_sequenceNumber++;
	return m_sequenceNumber-1;
}

int FreeEmsComms::getLocationIdInfo(unsigned short locationid)
{
	RequestClass req;
	req.type = GET_LOCATION_ID_INFO;
	req.sequencenumber = m_sequenceNumber;
	req.addArg(locationid,sizeof(locationid));
	req.sentRequest = true;
	req.hasReply = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}

int FreeEmsComms::getLocationIdList(unsigned char listtype, unsigned short listmask)
{
	RequestClass req;
	req.type = GET_LOCATION_ID_LIST;
	req.sequencenumber = m_sequenceNumber;
	req.addArg(listtype,sizeof(listtype));
	req.addArg(listmask,sizeof(listmask));
	req.sentRequest = true;
	req.hasReply = true;
	req.hasLength = true;
	m_sequenceNumber++;
	sendPacket(req);
	return m_sequenceNumber-1;
}

int FreeEmsComms::softReset()
{
	RequestClass req;
	req.type = SOFT_RESET;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	m_sequenceNumber++;
	return m_sequenceNumber-1;
}
int FreeEmsComms::hardReset()
{
	RequestClass req;
	req.type = HARD_RESET;
	req.sequencenumber = m_sequenceNumber;
	req.sentRequest = true;
	m_sequenceNumber++;
	return m_sequenceNumber-1;
}
bool FreeEmsComms::sendPacket(unsigned short payloadid)
{
    //Send simple packet
    RequestClass req;
    req.hasReply = true;
    req.type = (RequestType)payloadid;
    return sendPacket(req);
}

bool FreeEmsComms::sendPacket(RequestClass request)
{
	if (m_state != 1 && m_state != 2)
	{
		m_reqList.append(request);
		return false;
	}
	QLOG_DEBUG() << "sendPacket:" << "0x" + QString::number(request.type,16).toUpper();
	if (!request.hasReply)
	{
		return sendPacket(request.type,request.args,request.argsize,request.hasLength);
	}
	if (!m_waitingForResponse)
	{
		m_waitingForResponse = true;
		m_timeoutMsecs = QDateTime::currentDateTime().currentMSecsSinceEpoch();
		m_currentWaitingRequest = request;
		m_payloadWaitingForResponse = request.type;
		if (request.type == UPDATE_BLOCK_IN_RAM)
		{
			m_waitingForRamWrite = true;
		}
		if (request.type == UPDATE_BLOCK_IN_FLASH)
		{
			m_waitingForFlashWrite = true;
		}
		if (!sendPacket(request.type,request.args,request.argsize,request.hasLength))
		{
			return false;
		}
		return true;
	}
	else
	{
		QLOG_DEBUG() << "sendPacket packet queued:" << "0x" + QString::number(request.type,16).toUpper();
		m_reqList.append(request);
	}
	return false;

}

bool FreeEmsComms::sendPacket(unsigned short payloadid,QList<QVariant> arglist,QList<int> argsizelist,bool haslength)
{
	QByteArray payload = m_protocolEncoder.encodePacket(payloadid,arglist,argsizelist,haslength);
	QLOG_TRACE() << "About to send packet";
	m_lastMessageSentTime = QDateTime::currentMSecsSinceEpoch();
	if (serialPort->writeBytes(payload) < 0)
	{
		return false;
	}
	QLOG_TRACE() << "Sent packet" << "0x" + QString::number(payloadid,16).toUpper() <<  payload.size();
	//QLOG_DEBUG() << header.toHex() << payload.toHex();
	emit packetSent(payloadid,payload,payload);
	return true;
}
void FreeEmsComms::setInterByteSendDelay(int milliseconds)
{
	Q_UNUSED(milliseconds)
	//serialPort->setInterByteSendDelay(milliseconds);
}

void FreeEmsComms::run()
{
	//this->moveToThread(QThread::currentThread());
	//exec();
}
void FreeEmsComms::rxThreadPortGone()
{
	disconnectSerial();
	emit error("Serial port has disappeared. Save your tune (File->Save tune) then ensure the device is still connected and powered, then reconnect");
}

void FreeEmsComms::sendNextInterrogationPacket()
{
	if (m_interrogatePacketList.size() == 0)
	{
		if (!m_interrogateIdListComplete)
		{
			QLOG_DEBUG() << "Interrogation ID List complete" << m_locationIdList.size() << "entries";
			if (m_locationIdList.size() == 0)
			{
				//Things have gone terribly wrong here.
				//Throw out what we have, and allow interrogation to complete.
				emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
				emit interrogationComplete();
				emit interrogationData(m_interrogationMetaDataMap);
				m_interrogateInProgress = false;
				return;
			}
			m_interrogateIdListComplete = true;
			m_interrogateTotalCount += m_locationIdList.size();
			for (int i=0;i<m_locationIdList.size();i++)
			{
				int task = getLocationIdInfo(m_locationIdList[i]);
				m_interrogatePacketList.append(task);
				emit interrogateTaskStart("Location ID Info: 0x" + QString::number(m_locationIdList[i],16),task);
				//void interrogateTaskStart(QString task, int sequence);
				//void interrogateTaskFail(int sequence);
			}
			emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
		}
		else if (!m_interrogateIdInfoComplete)
		{
			//Fill out the parent information for both device, and local ram.
			emsData.populateDeviceRamAndFlashParents();
			emsData.populateLocalRamAndFlash();
			m_interrogateIdInfoComplete = true;
			QList<unsigned short> ramlist = emsData.getTopLevelDeviceRamLocations();
			QList<unsigned short> flashlist = emsData.getTopLevelDeviceFlashLocations();
			m_interrogateTotalCount += ramlist.size() + flashlist.size();
			if (ramlist.size() == 0 && flashlist.size() == 0)
			{
				//Things have gone terribly wrong here.
				//Throw out what we have, and allow interrogation to complete.
				emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
				emit interrogationComplete();
				emit interrogationData(m_interrogationMetaDataMap);
				m_interrogateInProgress = false;
				return;
			}
			for (int i=0;i<ramlist.size();i++)
			{
				int task = retrieveBlockFromRam(ramlist[i],0,0,false);
				m_interrogatePacketList.append(task);
				emit interrogateTaskStart("Ram Location: 0x" + QString::number(ramlist[i],16),task);
			}
			for (int i=0;i<flashlist.size();i++)
			{
				int task = retrieveBlockFromFlash(flashlist[i],0,0,false);
				m_interrogatePacketList.append(task);
				emit interrogateTaskStart("Flash Location: 0x" + QString::number(flashlist[i],16),task);
			}
			emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
		}
		else
		{
			QLOG_DEBUG() << "Interrogation complete";
			//Interrogation complete.
			emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
			emit interrogationComplete();
			m_interrogateInProgress = false;
			QLOG_DEBUG() << "Interrogate in progress is now false";
			for (int i=0;i<emsData.getUniqueLocationIdList().size();i++)
			{
				locationIdUpdate(emsData.getUniqueLocationIdList()[i]);
			}
			emit interrogationData(m_interrogationMetaDataMap);
			QString json = "";
			json += "{";
			/*QJson::Serializer jsonSerializer;
			QVariantMap top;
			for (QMap<QString,QString>::const_iterator i=m_interrogationMetaDataMap.constBegin();i!=m_interrogationMetaDataMap.constEnd();i++)
			{
				top[i.key()] = i.value();
			}*/
			/*top["firmwareversion"] = emsinfo.firmwareVersion;
			top["interfaceversion"] = emsinfo.interfaceVersion;
			top["compilerversion"] = emsinfo.compilerVersion;
			top["firmwarebuilddate"] = emsinfo.firmwareBuildDate;
			top["decodername"] = emsinfo.decoderName;
			top["operatingsystem"] = emsinfo.operatingSystem;
			top["emstudiohash"] = emsinfo.emstudioHash;
			top["emstudiocommit"] = emsinfo.emstudioCommit;*/

			/*if (m_logsEnabled)
			{
				QFile *settingsFile = new QFile(m_logsDirectory + "/" + m_logsFilename + ".meta.json");
				settingsFile->open(QIODevice::ReadWrite);
				settingsFile->write(jsonSerializer.serialize(top));
				settingsFile->close();
			}*/
			//deviceDataUpdated(unsigned short)
		}
	}
}

bool FreeEmsComms::sendSimplePacket(unsigned short payloadid)
{
	return sendPacket(payloadid);
}

void FreeEmsComms::packetNakedRec(unsigned short payloadid,QByteArray header,QByteArray payload,unsigned short errornum)
{
	QLOG_DEBUG() << "Packet nak" << "0x" + QString::number(payloadid,16).toUpper();
	//QMutexLocker locker(&m_waitingInfoMutex);
	if (m_waitingForResponse)
	{
		if (m_interrogateInProgress)
		{
			if (m_interrogatePacketList.contains(m_currentWaitingRequest.sequencenumber))
			{
				//Interrogate command failed!!
				emit interrogateTaskFail(m_currentWaitingRequest.sequencenumber);
				m_interrogatePacketList.removeOne(m_currentWaitingRequest.sequencenumber);
				emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
				sendNextInterrogationPacket();
				{
					if (m_payloadWaitingForResponse == GET_LOCATION_ID_LIST)
					{

					}
				}
			}
			else
			{
			}
		}
		if (m_waitingForFlashWrite)
		{
			m_waitingForFlashWrite = false;
			unsigned short locid = m_currentWaitingRequest.args[0].toUInt();
			emsData.setLocalFlashBlock(locid,emsData.getDeviceFlashBlock(locid));
			if (m_2dTableMap.contains(locid))
			{
				m_2dTableMap[locid]->setData(locid,!emsData.hasLocalRamBlock(locid),emsData.getLocalFlashBlock(locid));
			}
			if (m_3dTableMap.contains(locid))
			{
				m_3dTableMap[locid]->setData(locid,!emsData.hasLocalRamBlock(locid),emsData.getLocalFlashBlock(locid));
			}
			if (m_rawDataMap.contains(locid))
			{
				if (emsData.hasLocalRamBlock(locid))
				{
					m_rawDataMap[locid]->setData(locid,false,emsData.getLocalRamBlock(locid));
				}
				else
				{
					m_rawDataMap[locid]->setData(locid,true,emsData.getLocalFlashBlock(locid));
				}
			}
			if (m_locIdToConfigListMap.contains(locid))
			{
				for (int i=0;i<m_locIdToConfigListMap[locid].size();i++)
				{
					m_locIdToConfigListMap[locid][i]->setData(emsData.getLocalFlashBlock(locid));
				}
			}
		}
		if (m_waitingForRamWrite)
		{
			m_waitingForRamWrite = false;
			unsigned short locid = m_currentWaitingRequest.args[0].toUInt();
			emsData.setLocalRamBlock(locid,emsData.getDeviceRamBlock(locid));
			if (m_2dTableMap.contains(locid))
			{
				m_2dTableMap[locid]->setData(locid,!emsData.hasLocalRamBlock(locid),emsData.getLocalRamBlock(locid));
			}
			if (m_3dTableMap.contains(locid))
			{
				m_3dTableMap[locid]->setData(locid,!emsData.hasLocalRamBlock(locid),emsData.getLocalRamBlock(locid));
			}
			if (m_rawDataMap.contains(locid))
			{
				if (emsData.hasLocalRamBlock(locid))
				{
					m_rawDataMap[locid]->setData(locid,false,emsData.getLocalRamBlock(locid));
				}
				else
				{
					m_rawDataMap[locid]->setData(locid,true,emsData.getLocalFlashBlock(locid));
				}
			}
			if (m_locIdToConfigListMap.contains(locid))
			{
				for (int i=0;i<m_locIdToConfigListMap[locid].size();i++)
				{
					m_locIdToConfigListMap[locid][i]->setData(emsData.getLocalRamBlock(locid));
				}
			}

			unsigned short offset = m_currentWaitingRequest.args[1].toUInt();
			unsigned short size = m_currentWaitingRequest.args[2].toUInt();
			unsigned short ramaddress = emsData.getLocalRamBlockInfo(locid)->ramAddress;
			for (int i=ramaddress+offset;i<ramaddress+offset+size;i++)
			{
				if (m_dirtyRamAddresses.contains(i))
				{
					m_dirtyRamAddresses.removeOne(i);
				}
			}
			if (m_dirtyRamAddresses.size() == 0)
			{
				emit memoryClean();
			}

		}
		if (payloadid == m_payloadWaitingForResponse+1)
		{
			QLOG_TRACE() << "Recieved Negative Response" << "0x" + QString::number(m_payloadWaitingForResponse+1,16).toUpper() << "For Payload:" << "0x" + QString::number(m_payloadWaitingForResponse+1,16).toUpper()<< "Sequence Number:" << m_currentWaitingRequest.sequencenumber;
			QLOG_TRACE() << "Currently waiting for:" << QString::number(m_currentWaitingRequest.type,16).toUpper();
			//NAK to our packet
			//unsigned short errornum = parsedPacket.payload[0] << 8;
			//errornum += parsedPacket.payload[1];
			emit commandFailed(m_currentWaitingRequest.sequencenumber,errornum);
			emit packetNaked(m_currentWaitingRequest.type,header,payload,errornum);
			m_waitingForResponse = false;
			QTimer::singleShot(0,this,SLOT(triggerNextSend()));
		}
		else
		{
			QLOG_ERROR() << "ERROR! Invalid packet:" << "0x" + QString::number(payloadid,16).toUpper() << "Waiting for" << "0x" + QString::number(m_payloadWaitingForResponse+1,16).toUpper();
			QLOG_DEBUG() << payload.toHex();
		}
	}
	return;
	if (m_reqList.size() > 0)
	{
		QLOG_DEBUG() << "Sending next packet:" << "0x" + QString::number(m_reqList.at(0).type,16) << "Packets left:" << m_reqList.size();
		RequestClass req = m_reqList.at(0);
		m_reqList.removeAt(0);
		sendPacket(req);
	}
}
void FreeEmsComms::triggerNextSend()
{
	if (m_reqList.size() > 0)
	{
		QLOG_DEBUG() << "Sending next packet:" << "0x" + QString::number(m_reqList.at(0).type,16) << "Packets left:" << m_reqList.size();
		RequestClass req = m_reqList.at(0);
		m_reqList.removeAt(0);
		sendPacket(req);
	}
}

void FreeEmsComms::partialPacketAckedRec(unsigned short payloadid,QByteArray header,QByteArray payload)
{

	//Reset the timeout timer, and wait for more packets
	m_timeoutMsecs = QDateTime::currentDateTime().currentMSecsSinceEpoch();
	m_lastMessageSentTime = QDateTime::currentMSecsSinceEpoch();
	if (m_waitingForResponse)
	{
	}
}

void FreeEmsComms::completePacketAckedRec(unsigned short payloadid,QByteArray header,QByteArray payload)
{
	packetAckedRec(payloadid,header,payload);
}

void FreeEmsComms::packetAckedRec(unsigned short payloadid,QByteArray header,QByteArray payload)
{
	QLOG_DEBUG() << "Packet ack" << "0x" + QString::number(payloadid,16).toUpper();
	//QMutexLocker locker(&m_waitingInfoMutex);
	if (m_waitingForResponse)
	{
		if (m_interrogateInProgress)
		{
			if (m_interrogatePacketList.contains(m_currentWaitingRequest.sequencenumber))
			{
				QLOG_DEBUG() << "Sending next interrogation packet";
				emit interrogateTaskSucceed(m_currentWaitingRequest.sequencenumber);
				m_interrogatePacketList.removeOne(m_currentWaitingRequest.sequencenumber);
				emit interrogationProgress(m_interrogateTotalCount - m_interrogatePacketList.size(),m_interrogateTotalCount);
				sendNextInterrogationPacket();

			}
			else
			{
			}
		}
		if (m_waitingForFlashWrite)
		{
			m_waitingForFlashWrite = false;
			unsigned short locid = m_currentWaitingRequest.args[0].toUInt();
			emsData.setDeviceFlashBlock(locid,emsData.getLocalFlashBlock(locid));
			emit locationIdUpdate(locid);
		}
		if (m_waitingForRamWrite)
		{
			m_waitingForRamWrite = false;
			unsigned short locid = m_currentWaitingRequest.args[0].toUInt();
			//Change has been accepted, copy local ram to device ram
			emsData.setDeviceRamBlock(locid,emsData.getLocalRamBlock(locid));
			emit locationIdUpdate(locid);
		}
		if (payloadid == m_payloadWaitingForResponse+1)
		{
			if (payloadid == 0x1)
			{
				int stoper = 1;
			}
			QLOG_TRACE() << "Recieved Response" << "0x" + QString::number(m_payloadWaitingForResponse+1,16).toUpper() << "For Payload:" << "0x" + QString::number(m_payloadWaitingForResponse+1,16).toUpper()<< "Sequence Number:" << m_currentWaitingRequest.sequencenumber;
			QLOG_TRACE() << "Currently waiting for:" << QString::number(m_currentWaitingRequest.type,16).toUpper();
			//QLOG_DEBUG() << header.toHex() << payload.toHex();
			//Packet is good.
			emit commandSuccessful(m_currentWaitingRequest.sequencenumber);
			emit packetAcked(m_currentWaitingRequest.type,header,payload);
			m_waitingForResponse = false;
			QTimer::singleShot(0,this,SLOT(triggerNextSend()));
		}
		else if (payloadid != 0x0191)
		{
			QLOG_ERROR() << "ERROR! Invalid packet:" << "0x" + QString::number(payloadid,16).toUpper()  << "Waiting for" << "0x" + QString::number(m_payloadWaitingForResponse+1,16).toUpper();
			QLOG_DEBUG() << header.toHex() << payload.toHex();
			//m_reqList.prepend(m_currentWaitingRequest);
			m_waitingForResponse = false;
			QLOG_DEBUG() << "Resending packet:" << "0x" + QString::number(m_currentWaitingRequest.type,16) << "Packets left:" << m_reqList.size();
			sendPacket(m_currentWaitingRequest);
			return;
		}

	}
	return;
	if (m_reqList.size() > 0)
	{
		QLOG_DEBUG() << "Sending next packet:" << "0x" + QString::number(m_reqList.at(0).type,16) << "Packets left:" << m_reqList.size();
		RequestClass req = m_reqList.at(0);
		m_reqList.removeAt(0);
		sendPacket(req);
	}

}

void FreeEmsComms::setLogFileName(QString filename)
{
	m_logsFilename = filename;
}
void FreeEmsComms::datalogTimerTimeout()
{
	if (!m_lastDatalogUpdateEnabled)
	{
		return;
	}
	quint64 current = QDateTime::currentMSecsSinceEpoch() - m_lastDatalogTime;
	if (current > 1500)
	{
		if (m_interrogateInProgress)
		{
			//Give interrogation a bit more of a timeout between datalog packets.
			if (current > 5000)
			{
				m_isSilent = true;
				m_lastDatalogUpdateEnabled = false;
				emit emsSilenceStarted(m_lastDatalogTime);
			}
		}
		else
		{
			//It's been 1.5 seconds since our last datalog. We've likely either reset, or stopped responding.
			m_isSilent = true;
			m_lastDatalogUpdateEnabled = false;
			emit emsSilenceStarted(m_lastDatalogTime);
		}
	}
}
Table2DData* FreeEmsComms::get2DTableData(unsigned short locationid)
{
	if (!m_2dTableMap.contains(locationid))
	{
		//This is an error condition
		return 0;
	}
	return m_2dTableMap[locationid];

}

Table3DData* FreeEmsComms::get3DTableData(unsigned short locationid)
{
	if (!m_3dTableMap.contains(locationid))
	{
		//This is an error condition
		return 0;
	}
	return m_3dTableMap[locationid];
}
Table2DData* FreeEmsComms::get2DTableData(QString locationname)
{
	bool ok = false;
	unsigned short locid = locationname.toInt(&ok,16);
	if (!ok)
	{
		return 0;
	}
	if (!m_2dTableMap.contains(locid))
	{
		return 0;
	}
	return m_2dTableMap.value(locid);
}

Table3DData* FreeEmsComms::get3DTableData(QString locationname)
{
	bool ok = false;
	unsigned short locid = locationname.toInt(&ok,16);
	if (!ok)
	{
		return 0;
	}
	if (!m_3dTableMap.contains(locid))
	{
		return 0;
	}
	return m_3dTableMap.value(locid);
}

RawData* FreeEmsComms::getRawData(unsigned short locationid)
{
	if (!m_rawDataMap.contains(locationid))
	{
		//This is an error condition
		return 0;
	}
	return m_rawDataMap[locationid];
}
ConfigData* FreeEmsComms::getConfigData(QString name)
{
    if (!m_configMap.contains(name))
    {
	return 0;
    }
    return m_configMap[name];
}
QList<QString> FreeEmsComms::getConfigList()
{
    return m_configMap.keys();
}

void FreeEmsComms::locationIdListRec(QList<unsigned short> locationidlist)
{
	m_locationIdList.clear();
	for (int i=0;i<locationidlist.size();i++)
	{
		m_locationIdList.append(locationidlist[i]);
	}
	emit locationIdList(m_locationIdList);
	QLOG_DEBUG() << m_locationIdList.size() << "Locations loaded";
}

void FreeEmsComms::locationIdUpdate(unsigned short locationid)
{
	//emsData.getChildrenOfLocalRamLocation()
	QList<unsigned short> updatelist;
	updatelist.append(locationid);
	if (emsData.localFlashHasParent(locationid))
	{
		updatelist.append(emsData.getParentOfLocalFlashLocation(locationid));
	}
	if (emsData.localRamHasParent(locationid))
	{
		updatelist.append(emsData.getParentOfLocalRamLocation(locationid));
	}
	if (emsData.localFlashHasChildren(locationid))
	{
		updatelist.append(emsData.getChildrenOfLocalFlashLocation(locationid));
	}
	if (emsData.localRamHasChildren(locationid))
	{
		updatelist.append(emsData.getChildrenOfLocalRamLocation(locationid));
	}

	for (int i=0;i<updatelist.size();i++)
	{
		//QLOG_DEBUG() << "Updating location id:" << QString::number(updatelist[i],16);
		if (m_2dTableMap.contains(updatelist[i]))
		{
			m_2dTableMap[updatelist[i]]->setData(updatelist[i],!emsData.hasDeviceRamBlock(updatelist[i]),emsData.getDeviceRamBlock(updatelist[i]));
		}
		if (m_3dTableMap.contains(updatelist[i]))
		{
			m_3dTableMap[updatelist[i]]->setData(updatelist[i],!emsData.hasDeviceRamBlock(updatelist[i]),emsData.getDeviceRamBlock(updatelist[i]));
		}
		if (m_rawDataMap.contains(updatelist[i]))
		{
			if (emsData.hasLocalRamBlock(updatelist[i]))
			{
				m_rawDataMap[updatelist[i]]->setData(updatelist[i],false,emsData.getLocalRamBlock(updatelist[i]));
			}
			else
			{
				m_rawDataMap[updatelist[i]]->setData(updatelist[i],true,emsData.getLocalFlashBlock(updatelist[i]));
			}
		}

		if (m_locIdToConfigListMap.contains(updatelist[i]))
		{
			for (int j=0;j<m_locIdToConfigListMap[updatelist[i]].size();j++)
			{
				m_locIdToConfigListMap[updatelist[i]][j]->setData(emsData.getLocalFlashBlock(updatelist[i]));
			}
		}
	}
}
void FreeEmsComms::copyFlashToRam(unsigned short locationid)
{
	emsData.setLocalRamBlock(locationid,emsData.getLocalFlashBlock(locationid));
	if (m_2dTableMap.contains(locationid))
	{
		m_2dTableMap[locationid]->setData(locationid,!emsData.hasLocalRamBlock(locationid),emsData.getLocalRamBlock(locationid));
	}
	if (m_3dTableMap.contains(locationid))
	{
		m_3dTableMap[locationid]->setData(locationid,!emsData.hasLocalRamBlock(locationid),emsData.getLocalRamBlock(locationid));
	}
	if (m_rawDataMap.contains(locationid))
	{
		if (emsData.hasLocalRamBlock(locationid))
		{
			m_rawDataMap[locationid]->setData(locationid,false,emsData.getLocalRamBlock(locationid));
		}
		else
		{
			m_rawDataMap[locationid]->setData(locationid,true,emsData.getLocalFlashBlock(locationid));
		}
	}

	if (m_locIdToConfigListMap.contains(locationid))
	{
		for (int i=0;i<m_locIdToConfigListMap[locationid].size();i++)
		{
			m_locIdToConfigListMap[locationid][i]->setData(emsData.getLocalRamBlock(locationid));
		}
	}
	updateBlockInRam(locationid,0,emsData.getLocalFlashBlock(locationid).size(),emsData.getLocalFlashBlock(locationid));

	for (int i=emsData.getLocalRamBlockInfo(locationid)->ramAddress;i<emsData.getLocalRamBlockInfo(locationid)->ramAddress+emsData.getLocalRamBlockInfo(locationid)->size;i++)
	{
		if (m_dirtyRamAddresses.contains(i))
		{
			m_dirtyRamAddresses.removeOne(i);
		}
	}
	if (m_dirtyRamAddresses.size() == 0)
	{
		emit memoryClean();
	}
}

void FreeEmsComms::copyRamToFlash(unsigned short locationid)
{
	emsData.setLocalFlashBlock(locationid,emsData.getLocalRamBlock(locationid));
	emsData.setDeviceFlashBlock(locationid,emsData.getDeviceRamBlock(locationid));
	burnBlockFromRamToFlash(locationid,0,0);
}

void FreeEmsComms::dataLogWrite(QByteArray buffer)
{
	if (m_logsEnabled)
	{
		if (m_debugLogsEnabled)
		{
			m_logOutFile->write(buffer);
			m_logInOutFile->write(buffer);
		}
	}
}


void FreeEmsComms::dataLogRead(QByteArray buffer)
{
	if (m_logsEnabled)
	{
		m_logInFile->write(buffer);
		if (m_debugLogsEnabled)
		{
			m_logInOutFile->write(buffer);
		}
	}
}

void FreeEmsComms::parseEverything(QByteArray buffer)
{
	if (!buffer.startsWith(QByteArray().append(0x01).append(0x01).append(0x91)))
	{
		QLOG_DEBUG() << "Incoming packet";
		//QLOG_DEBUG() << "parseEverything:" << buffer.toHex();
	}

	Packet p = m_packetDecoder->parseBuffer(buffer);
	if (p.payloadid != 0x191)
	{
		QLOG_DEBUG() << "Incoming packet" << p.isValid << "0x" + QString::number(p.payloadid,16).toUpper();
	}
	if (!p.isValid)
	{
		QLOG_DEBUG() << "Decoder failure:" << buffer.toHex();
		emit decoderFailure(buffer);
	}
	else
	{
		if (m_state == 1)
		{
			//Waiting on first FW version reply packet
			if (p.payloadid == 0x0003)
			{
				//Good to go!
				m_state = 2; //2 is running, kick off the next packet send.
				m_waitingForResponse = false; //Clear this out, since we're not parsing the packet direct.
				emit connected();
				QLOG_DEBUG() << "Connected...." << m_reqList.size() << "Items in queue";
			}
		}
		else if (m_state == 2)
		{
			m_packetDecoder->parsePacket(p);
		}
		//parsePacket(p);
	}
}
void FreeEmsComms::ramBlockUpdateRec(QByteArray header,QByteArray payload)
{
	if (m_currentWaitingRequest.args.size() == 0)
	{
		QLOG_ERROR() << "ERROR! Current waiting packet's arg size is zero1!!";
		QLOG_ERROR() << "0x" + QString::number(m_currentWaitingRequest.type,16).toUpper();
		//QLOG_ERROR() << "0x" + QString::number(payloadid,16).toUpper();
	}
	unsigned short locationid = m_currentWaitingRequest.args[0].toInt();
	emsData.ramBlockUpdate(locationid,header,payload);
}

void FreeEmsComms::flashBlockUpdateRec(QByteArray header,QByteArray payload)
{
	if (m_currentWaitingRequest.args.size() == 0)
	{
		QLOG_ERROR() << "ERROR! Current waiting packet's arg size is zero1!!";
		QLOG_ERROR() << "0x" + QString::number(m_currentWaitingRequest.type,16).toUpper();
		//QLOG_ERROR() << "0x" + QString::number(payloadid,16).toUpper();
	}
	unsigned short locationid = m_currentWaitingRequest.args[0].toInt();
	emsData.flashBlockUpdate(locationid,header,payload);
}

void FreeEmsComms::locationIdInfoRec(MemoryLocationInfo info)
{
	if (m_currentWaitingRequest.args.size() == 0)
	{
		QLOG_ERROR() << "ERROR! Current waiting packet's arg size is zero1!!";
		QLOG_ERROR() << "0x" + QString::number(m_currentWaitingRequest.type,16).toUpper();
		//QLOG_ERROR() << "0x" + QString::number(payloadid,16).toUpper();
	}
	unsigned short locationid = m_currentWaitingRequest.args[0].toInt();
	info.locationid = locationid;
	TableMeta tableMeta;
	FieldMeta xMeta;
	FieldMeta yMeta;
	FieldMeta zMeta;
	if (m_tableMetaMap.contains(info.descid))
	{
		//We have metadata!
		tableMeta = m_tableMetaMap.value(info.descid);
		tableMeta.size = info.size;
		m_tableMetaMap.remove(info.descid);
		m_tableMetaMap.insert(info.descid,tableMeta);
		xMeta = m_fieldMetaMap.value(tableMeta.xAxisId);
		yMeta = m_fieldMetaMap.value(tableMeta.yAxisId);
		zMeta = m_fieldMetaMap.value(tableMeta.zAxisId);
		info.metaData = tableMeta;
		info.xAxis = xMeta;
		info.yAxis = yMeta;
		info.zAxis = zMeta;
	}
	else
	{
		tableMeta.valid = false;
		info.metaData = tableMeta;
	}

	emit locationIdInfo(locationid,info);
	emsData.passLocationInfo(locationid,info);
	QLOG_DEBUG() << "Got memory location:" << info.locationid;
	if ((tableMeta.formatId == TABLE_2D_STRUCTURED || tableMeta.formatId == TABLE_2D_LEGACY) && tableMeta.valid)
	{
		Table2DData *data = 0;
		if (xMeta.size == 32)
		{
			data = new FETable2DData(true);
		}
		else
		{
			data = new FETable2DData(false);
		}
		data->setMetaData(tableMeta,xMeta,zMeta);
		connect(data,SIGNAL(saveSingleDataToRam(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(ramBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
		connect(data,SIGNAL(saveSingleDataToFlash(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(flashBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
		connect(data,SIGNAL(requestBlockFromRam(unsigned short,unsigned short,unsigned short)),this,SLOT(retrieveBlockFromRam(unsigned short,unsigned short,unsigned short)));
		connect(data,SIGNAL(requestBlockFromFlash(unsigned short,unsigned short,unsigned short)),this,SLOT(retrieveBlockFromFlash(unsigned short,unsigned short,unsigned short)));
		connect(data,SIGNAL(requestRamUpdateFromFlash(unsigned short)),this,SLOT(copyFlashToRam(unsigned short)));
		connect(data,SIGNAL(requestFlashUpdateFromRam(unsigned short)),this,SLOT(copyRamToFlash(unsigned short)));
		m_2dTableMap[locationid] = data;
		return;
	}
	else if (tableMeta.formatId == TABLE_3D && tableMeta.valid)
	{
		Table3DData *data = new FETable3DData();
		data->setMetaData(tableMeta,xMeta,yMeta,zMeta);
		connect(data,SIGNAL(saveSingleDataToRam(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(ramBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
		connect(data,SIGNAL(saveSingleDataToFlash(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(flashBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
		connect(data,SIGNAL(requestBlockFromRam(unsigned short,unsigned short,unsigned short)),this,SLOT(retrieveBlockFromRam(unsigned short,unsigned short,unsigned short)));
		connect(data,SIGNAL(requestBlockFromFlash(unsigned short,unsigned short,unsigned short)),this,SLOT(retrieveBlockFromFlash(unsigned short,unsigned short,unsigned short)));
		connect(data,SIGNAL(requestRamUpdateFromFlash(unsigned short)),this,SLOT(copyFlashToRam(unsigned short)));
		connect(data,SIGNAL(requestFlashUpdateFromRam(unsigned short)),this,SLOT(copyRamToFlash(unsigned short)));
		m_3dTableMap[locationid] = data;
		return;
	}
	RawData *data = new FERawData();
	connect(data,SIGNAL(saveSingleDataToRam(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(ramBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
	connect(data,SIGNAL(saveSingleDataToFlash(unsigned short,unsigned short,unsigned short,QByteArray)),&emsData,SLOT(flashBytesLocalUpdate(unsigned short,unsigned short,unsigned short,QByteArray)));
	connect(data,SIGNAL(requestBlockFromRam(unsigned short,unsigned short,unsigned short)),this,SLOT(retrieveBlockFromRam(unsigned short,unsigned short,unsigned short)));
	connect(data,SIGNAL(requestBlockFromFlash(unsigned short,unsigned short,unsigned short)),this,SLOT(retrieveBlockFromFlash(unsigned short,unsigned short,unsigned short)));
	m_rawDataMap[locationid] = data;
}

void FreeEmsComms::ramLocationMarkedDirty(unsigned short locationid)
{
	emit ramLocationDirty(locationid);
}

void FreeEmsComms::flashLocationMarkedDirty(unsigned short locationid)
{
	emit flashLocationDirty(locationid);

}
void FreeEmsComms::acceptLocalChanges()
{
	for (int i=0;i<emsData.getDirtyRamLocations().size();i++)
	{
		updateBlockInRam(emsData.getDirtyRamLocations().at(i).first,0,emsData.getDirtyRamLocations().at(i).second.size(),emsData.getDirtyRamLocations().at(i).second);
	}
	emsData.clearDirtyRamLocations();
}
void FreeEmsComms::rejectLocalChanges()
{
	emsData.clearDirtyRamLocations();
}
void FreeEmsComms::dataLogPayloadReceivedRec(QByteArray header,QByteArray payload)
{
	//Incoming datalogpacket, reset the timer.
	m_lastDatalogTime = QDateTime::currentMSecsSinceEpoch();
	if (!m_lastDatalogUpdateEnabled)
	{
		m_lastDatalogUpdateEnabled = true;
		emit emsSilenceBroken();
	}
}
void FreeEmsComms::timeoutTimerTick()
{
	if (m_waitingForResponse)
	{
		if (QDateTime::currentMSecsSinceEpoch() - m_lastMessageSentTime > 1000)
		{
			//It's been 1 second since the last sent message with no response, resend!
			//Greater than 1 second, resend!
			m_waitingForResponse = false;
			QLOG_DEBUG() << "Resending packet:" << "0x" + QString::number(m_currentWaitingRequest.type,16) << "Packets left:" << m_reqList.size();
			sendPacket(m_currentWaitingRequest);
		}
	}
}

void FreeEmsComms::saveDatalogDescriptor(QString json)
{
	QFile jsonfile(m_logsDirectory + "/" + m_logsFilename + ".json");
	jsonfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
	jsonfile.write(json.toLatin1());
	jsonfile.close();
}
void FreeEmsComms::fieldDescriptor(QString json)
{
	QFile jsonfile(m_logsDirectory + "/" + m_logsFilename + ".fields.json");
	jsonfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
	jsonfile.write(json.toLatin1());
	jsonfile.close();

	QJsonDocument document = QJsonDocument::fromJson(json.toLatin1());
	if (document.isEmpty())
	{
		//qDebug() << "Unable to parse datalog descriptor json:" << parser.errorString();
		return;
	}
	QJsonObject toplevelobject = document.object();
	QJsonArray descriptorlist = toplevelobject.value("descriptor").toArray();
	for (int i=0;i<descriptorlist.size();i++)
	{
		QJsonObject itemmap = descriptorlist.at(i).toObject();
		int id = itemmap.value("id").toInt();
		int size = itemmap.value("size").toInt();
		int issigned = itemmap.value("is_signed").toInt();
		//int locid = itemmap.value("locationId").toInt();
		QString name = itemmap.value("name").toString();
		QString desc = itemmap.value("description").toString();
		double mult = itemmap.value("multiplier").toString().toDouble();
		double adder = itemmap.value("adder").toString().toDouble();
		QString transfer = itemmap.value("transfer_function").toString();
		QString flags = itemmap.value("flags").toString();
		QString suffix = itemmap.value("suffix").toString();

		FieldMeta meta;

		meta.id = id;
		meta.size = size;
		meta.isSigned = issigned;
		meta.name = name;
		meta.desc = desc;
		meta.multiplier = mult;
		meta.adder = adder;
		meta.trasnfer = transfer;
		meta.flags = flags;
		meta.suffix = suffix;
		meta.valid = true;
		m_fieldMetaMap[id] = meta;
	}
}
void FreeEmsComms::tableDescriptor(QString json)
{
	QFile jsonfile(m_logsDirectory + "/" + m_logsFilename + ".tables.json");
	jsonfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
	jsonfile.write(json.toLatin1());
	jsonfile.close();

	QJsonDocument document = QJsonDocument::fromJson(json.toLatin1());
	if (document.isEmpty())
	{
		//qDebug() << "Unable to parse datalog descriptor json:" << parser.errorString();
		return;
	}
	QJsonObject toplevelobject = document.object();
	QJsonArray descriptorlist = toplevelobject.value("descriptor").toArray();
	for (int i=0;i<descriptorlist.size();i++)
	{
		QJsonObject itemmap = descriptorlist.at(i).toObject();
		int id = itemmap.value("id").toString().toInt();
		//int locid = itemmap.value("locationId").toInt();
		QString name = itemmap.value("name").toString();
		QString desc = itemmap.value("description").toString();
		int format_id = itemmap.value("format_id").toString().toInt();
		int xAxis_id = itemmap.value("xAxisID").toString().toInt();
		int yAxis_id = itemmap.value("yAxisID").toString().toInt();
		int zAxis_id = itemmap.value("lookupID").toString().toInt();

		TableMeta meta;
		meta.id = id;
		//meta.locationId = locid;
		meta.name = name;
		meta.desc = desc;
		meta.formatId = format_id;
		meta.xAxisId = xAxis_id;
		meta.yAxisId = yAxis_id;
		meta.zAxisId = zAxis_id;
		meta.valid = true;
		m_tableMetaMap[id] = meta;
	}
}

void FreeEmsComms::serialPortDisconnected()
{
	m_state = 1;
	m_lastDatalogUpdateEnabled = false;
	serialPort->wait(2000);
	serialPort->terminate();
	delete serialPort;
	serialPort = 0;
	emit disconnected();

}

void FreeEmsComms::serialPortError(QString errorstr)
{
	emit error(errorstr);
}
void FreeEmsComms::firmwareDebug(QString text)
{
	emit firmwareDebugReceived(text);
}
