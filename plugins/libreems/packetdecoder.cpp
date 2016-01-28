#include "packetdecoder.h"
#include "QsLog.h"
#include <QMutexLocker>
#include "memorylocationinfo.h"
#include <QDateTime>
#include <QFile>
#define NAK 0x02

PacketDecoder::PacketDecoder(QObject *parent) : QObject(parent)
{
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
	isPartialPacketWaiting = false;
}

void PacketDecoder::parseBufferToPacket(QByteArray buffer)
{
	Packet p = parseBuffer(buffer);
	parsePacket(p);
}

Packet PacketDecoder::parseBuffer(QByteArray buffer)
{
	if (buffer.size() <= 2)
	{

		QLOG_ERROR() << "Not long enough to even contain a header!";
		//emit decoderFailure(buffer);
		return Packet(false);
	}
	buffer = buffer.mid(0,buffer.size()-1);


	Packet retval;
	QByteArray header;
	//Parse the packet here
	int headersize = 3;
	int iloc = 0;
	bool seq = false;
	bool len = false;
	bool ispartial = false;
	bool iscomplete = false;
	if (buffer[iloc] & 0x4)
	{
		//Has seq
		seq = true;
		headersize += 1;
	}
	if (buffer[iloc] & 0x1)
	{
		//Has length
		len = true;
		headersize += 2;
	}
	if (buffer[iloc] & 0x8)
	{
		//Is partial packet
		ispartial = true;
		headersize += 1;

	}
	if (buffer[iloc] & 0x16)
	{
		//Is final packet of a partial sequence
		iscomplete = true;
		headersize += 1;
	}
	header = buffer.mid(0,headersize);
	iloc++;
	unsigned int payloadid = (unsigned int)buffer[iloc] << 8;

	payloadid += (unsigned char)buffer[iloc+1];
	retval.payloadid = payloadid;
	iloc += 2;
	if (seq)
	{
		iloc += 1;
		retval.hasseq = true;
	}
	else
	{
		retval.hasseq = false;
	}
	QByteArray payload;
	if (len)
	{
		retval.haslength = true;
		unsigned int length = buffer[iloc] << 8;
		length += (unsigned char)buffer[iloc+1];
		retval.length = length;
		iloc += 2;
	}
	else
	{
		retval.haslength = false;
	}
	if (ispartial)
	{
		retval.isPartial = true;
		retval.partialSequence = (unsigned char)buffer[iloc];
		iloc += 1;
	}
	else
	{
		retval.isPartial = false;
	}
	if (iscomplete)
	{
		retval.isComplete = true;
		retval.partialSequence = (unsigned char)buffer[iloc];
		iloc += 1;
	}
	else
	{
		retval.isComplete = false;
	}
	if (retval.haslength)
	{
		if ((unsigned int)buffer.length() > (unsigned int)(retval.length + iloc))
		{
			QLOG_ERROR() << "Packet length should be:" << retval.length + iloc << "But it is" << buffer.length();
			//emit decoderFailure(buffer);
			return Packet(false);
		}
		payload.append(buffer.mid(iloc,retval.length));
	}
	else
	{
		payload.append(buffer.mid(iloc),(buffer.length()-iloc));
	}

	/*QString output;
	for (int i=0;i<payload.size();i++)
	{
		int num = (unsigned char)payload[i];
		output.append(" ").append((num < 0xF) ? "0" : "").append(QString::number(num,16));
	}
	output.clear();
	for (int i=0;i<header.size();i++)
	{
		int num = (unsigned char)header[i];
		output.append(" ").append((num < 0xF) ? "0" : "").append(QString::number(num,16));
	}*/
	//Last byte of currPacket should be out checksum.
	retval.header = header;
	retval.payload = payload;
	if (header[0] & NAK)
	{
		retval.isNAK = true;
	}
	else
	{
		retval.isNAK = false;
	}
	if (retval.header.size() >= 3)
	{
		return retval;
	}
	else
	{
		QLOG_ERROR() << "Packet header under minimum header size" << retval.header.size();
		return Packet(false);
	}
}
void PacketDecoder::parsePacket(Packet parsedPacket)
{
	if (parsedPacket.isValid)
	{
		//QMutexLocker locker(&m_waitingInfoMutex);
		unsigned short payloadid = parsedPacket.payloadid;

		//if (m_isSilent)
		//{
		//	emit emsSilenceBroken();
		//	m_isSilent = false;
		//}
		if (payloadid == 0x0191)
		{	//Datalog packet

			if (parsedPacket.isNAK)
			{
				//NAK
			}
			else
			{
				emit dataLogPayloadReceived(parsedPacket.header,parsedPacket.payload);
			}
		}
		else if (payloadid == 0xEEEF)
		{
			//Decoder
			if (!(parsedPacket.isNAK))
			{
				emit decoderName(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0xEEF1)
		{
			//Firmware build date
			if (!(parsedPacket.isNAK))
			{
				emit firmwareBuild(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0xEEF3)
		{
			//Compiler Version
			if (!(parsedPacket.isNAK))
			{
				emit compilerVersion(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0xEEF5)
		{
			//Operating System
			if (!(parsedPacket.isNAK))
			{
				emit operatingSystem(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0xEEF7)
		{
			//Built By Name
			if (!(parsedPacket.isNAK))
			{
				emit builtByName(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0xEEF9)
		{
			//Support Email
			if (!(parsedPacket.isNAK))
			{
				emit supportEmail(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0xDA5F)
		{
			//Location ID List
			if (parsedPacket.isNAK)
			{
			}
			else
			{
				//TODO double check to make sure that there aren't an odd number of items here...
				QLOG_DEBUG() << "Location ID List";
				QString details = "Details: {";
				for (int j=0;j<parsedPacket.payload.size();j++)
				{
					details += "0x";
					details += (((unsigned char)parsedPacket.payload[j] < 0xF) ? "0" : "");
					details += QString::number(parsedPacket.payload[j],16);
					details += ",";
				}
				details += "}";
				QList<unsigned short> locationidlist;
				for (int j=0;j<parsedPacket.payload.size();j+=2)
				{
					unsigned short tmp = 0;
					tmp += parsedPacket.payload[j] << 8;
					tmp += parsedPacket.payload[j+1];
					locationidlist.append(tmp);
				}
				emit locationIdList(locationidlist);
			}
		}
		else if (payloadid == 0xF8E1) //Location ID Info
		{
			if (parsedPacket.isNAK)
			{
			}
			else
			{
				QString details = "Details: {";
				for (int j=0;j<parsedPacket.payload.size();j++)
				{
					details += "0x";
					details += (((unsigned char)parsedPacket.payload[j] < 0xF) ? "0" : "");
					details += QString::number(parsedPacket.payload[j],16);
					details += ",";
				}
				details += "}";


				//unsigned short locationid = m_currentWaitingRequest.args[0].toInt();
				//TODO double check to make sure that there aren't an odd number of items here...
				QList<LocationIdFlags> flaglist;
				if (parsedPacket.payload.size() >= 2)
				{
					MemoryLocationInfo info;
					unsigned short test = parsedPacket.payload[0] << 8;
					unsigned short parent;
					unsigned char rampage;
					unsigned char flashpage;
					unsigned short ramaddress;
					unsigned short flashaddress;
					unsigned short size;
					unsigned short descid;
					test += parsedPacket.payload[1];


					for (int j=0;j<m_blockFlagList.size();j++)
					{
						if (test & m_blockFlagList[j])
						{
							flaglist.append(m_blockFlagList[j]);
							if (m_blockFlagToNameMap.contains(m_blockFlagList[j]))
							{
								info.propertymap.append(QPair<QString,QString>(m_blockFlagToNameMap[m_blockFlagList[j]],"true"));
							}
						}
						else
						{
							if (m_blockFlagToNameMap.contains(m_blockFlagList[j]))
							{
								info.propertymap.append(QPair<QString,QString>(m_blockFlagToNameMap[m_blockFlagList[j]],"false"));
							}
						}
					}
					parent = ((unsigned char)parsedPacket.payload[2]) << 8;
					parent += (unsigned char)parsedPacket.payload[3];
					rampage = (unsigned char)parsedPacket.payload[4];
					flashpage = (unsigned char)parsedPacket.payload[5];
					ramaddress = ((unsigned char)parsedPacket.payload[6]) << 8;
					ramaddress += (unsigned char)parsedPacket.payload[7];
					flashaddress = ((unsigned char)parsedPacket.payload[8]) << 8;
					flashaddress += (unsigned char)parsedPacket.payload[9];
					size = ((unsigned char)parsedPacket.payload[10]) << 8;
					size += (unsigned char)parsedPacket.payload[11];
					//descid = ((unsigned char)parsedPacket.payload[12]) << 8;
					//descid += (unsigned char)parsedPacket.payload[13];
					descid = (unsigned char)parsedPacket.payload[12];

					//info.locationid = locationid;
					info.parent = parent;
					info.ramaddress = ramaddress;
					info.rampage = rampage;
					info.flashaddress = flashaddress;
					info.flashpage = flashpage;
					info.rawflags = test;
					info.size = size;
					info.descid = descid;

					info.propertymap.append(QPair<QString,QString>("parent","0x" + QString::number(parent,16).toUpper()));
					info.propertymap.append(QPair<QString,QString>("rampage","0x" + QString::number(rampage,16).toUpper()));
					info.propertymap.append(QPair<QString,QString>("flashpage","0x" + QString::number(flashpage,16).toUpper()));
					info.propertymap.append(QPair<QString,QString>("ramaddress","0x" + QString::number(ramaddress,16).toUpper()));
					info.propertymap.append(QPair<QString,QString>("flashaddress","0x" + QString::number(flashaddress,16).toUpper()));
					info.propertymap.append(QPair<QString,QString>("size","0x" + QString::number(size,16).toUpper()));

					if (flaglist.contains(BLOCK_IS_RAM))
					{
						info.isRam = true;
					}
					else
					{
						info.isRam = false;
					}
					if (flaglist.contains(BLOCK_IS_FLASH))
					{
						info.isFlash = true;
					}
					else
					{
						info.isFlash = false;
					}
					if (flaglist.contains(BLOCK_HAS_PARENT))
					{
						info.hasParent = true;
					}
					else
					{
						info.hasParent = false;
					}
					if (flaglist.contains(BLOCK_IS_READ_ONLY))
					{
						info.isReadOnly = true;
					}
					QString flags = "flags for table: " + QString::number(info.ramaddress,16).toUpper() + " : ";
					for (int i=0;i<flaglist.size();i++)
					{
						flags += QString::number(flaglist.at(i),16).toUpper() + ",";
					}
					qDebug() << flags;
					if (flaglist.contains(BLOCK_SPARE_10))
					{
						info.type = DATA_TABLE;
					}
					else if (flaglist.contains(BLOCK_IS_2D_TABLE))
					{
						info.type = DATA_TABLE;
					}
					else if (flaglist.contains(BLOCK_IS_2D_SIGNED_TABLE))
					{
						info.type = DATA_TABLE;
					}
					else if (flaglist.contains(BLOCK_IS_MAIN_TABLE))
					{
						//info.type = DATA_TABLE_3D;
						info.type = DATA_TABLE;
					}
					else if (flaglist.contains(BLOCK_IS_CONFIGURATION))
					{
						info.type = DATA_CONFIG;
					}
					else if (flaglist.contains(BLOCK_IS_LOOKUP_DATA))
					{
						info.type = DATA_TABLE_LOOKUP;
					}
					else
					{
						info.type = DATA_UNDEFINED;
					}
					emit locationIdInfo(info);
					//emsData.passLocationInfo(locationid,info);

				}


			}
		}
		else if (payloadid == 0x0001) //Interface version response
		{
			//Handle interface version
			if (parsedPacket.isNAK)
			{
				//NAK
				QLOG_ERROR() << "IFACE VERSION NAK";
			}
			else
			{
				emit interfaceVersion(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0x0003) //Firmware version response
		{
			if (parsedPacket.isNAK)
			{
				//NAK
				QLOG_ERROR() << "FIRMWARE VERSION NAK";
			}
			else
			{
				emit firmwareVersion(QString(parsedPacket.payload));
			}
		}
		else if (payloadid == 0x0107)
		{
			if (parsedPacket.isNAK)
			{
			}
			else
			{
				//unsigned short locid = m_currentWaitingRequest.args[0].toUInt();
				//emit flashBlockRetrieved(locid,parsedPacket.header,parsedPacket.payload);
				//emsData.flashBlockUpdate(locid,parsedPacket.header,parsedPacket.payload);
				emit flashBlockUpdatePacket(parsedPacket.header,parsedPacket.payload);
			}
		}
		else if (payloadid == 0x0105)
		{
			if (parsedPacket.isNAK)
			{
			}
			else
			{
				//Block from ram is here.
				//unsigned short locid = m_currentWaitingRequest.args[0].toUInt();
				//emit ramBlockRetrieved(locid,parsedPacket.header,parsedPacket.payload);
				//emsData.ramBlockUpdate(locid,parsedPacket.header,parsedPacket.payload);
				emit ramBlockUpdatePacket(parsedPacket.header,parsedPacket.payload);
			}
		}
		else if (payloadid == 0x0301)
		{
			qDebug() << "Datalog info packet:" << parsedPacket.header.toHex();
			if (!isPartialPacketWaiting)
			{
				isPartialPacketWaiting = true;
				m_partialPacketBuffer.clear();
				m_currentPartialPacketSequence = parsedPacket.partialSequence-1;
			}
			if (m_currentPartialPacketSequence != parsedPacket.partialSequence-1)
			{
				//Current is not directly after!!!
			}
			m_partialPacketBuffer.append(parsedPacket.payload);
			m_currentPartialPacketSequence = parsedPacket.partialSequence;
			if (parsedPacket.isComplete)
			{
				//Final in sequence
				emit datalogDescriptor(m_partialPacketBuffer);
				isPartialPacketWaiting=false;
			}

		}
		else if (payloadid == 0x030D)
		{
			qDebug() << "Field Descriptor packet:" << parsedPacket.header.toHex();
			//Field descriptor
			if (!isPartialPacketWaiting)
			{
				isPartialPacketWaiting = true;
				m_partialPacketBuffer.clear();
				m_currentPartialPacketSequence = parsedPacket.partialSequence-1;
			}
			if (m_currentPartialPacketSequence != parsedPacket.partialSequence-1)
			{
				//Current is not directly after!!!
			}
			m_partialPacketBuffer.append(parsedPacket.payload);
			m_currentPartialPacketSequence = parsedPacket.partialSequence;
			if (parsedPacket.isComplete)
			{
				//Final in sequence
				emit fieldDescriptor(m_partialPacketBuffer);
				isPartialPacketWaiting=false;
			}
		}
		else if (payloadid == 0x030F)
		{
			qDebug() << "Table Descriptor packet:" << parsedPacket.header.toHex();
			//Field descriptor
			if (!isPartialPacketWaiting)
			{
				isPartialPacketWaiting = true;
				m_partialPacketBuffer.clear();
				m_currentPartialPacketSequence = parsedPacket.partialSequence-1;
			}
			if (m_currentPartialPacketSequence != parsedPacket.partialSequence-1)
			{
				//Current is not directly after!!!
			}
			m_partialPacketBuffer.append(parsedPacket.payload);
			m_currentPartialPacketSequence = parsedPacket.partialSequence;
			if (parsedPacket.isComplete)
			{
				//Final in sequence
				emit tableDescriptor(m_partialPacketBuffer);

				//QFile jsonfile("C:\\Users\Michael\\EMStudio\\logs\\2016.01.11-05.28.15.tables.json");
				//jsonfile.open(QIODevice::ReadOnly);
				//jsonfile.write(json.toLatin1());
				//QByteArray partial = jsonfile.readAll();
				//jsonfile.close();
				//emit tableDescriptor(partial);
				isPartialPacketWaiting=false;
			}
		}
		else if (payloadid == BENCHTEST+1)
		{
			if (!parsedPacket.isNAK)
			{
				if (parsedPacket.payload.size() == 3)
				{
					unsigned short count = 0;
					count += ((unsigned char)parsedPacket.payload.at(0)) << 8;
					count += ((unsigned char)parsedPacket.payload.at(1));
					unsigned char event = 0;
					event = ((unsigned char)parsedPacket.payload.at(2));
					emit benchTestReply(count,event);
				}
			}
		}
		else
		{
			emit unknownPacket(parsedPacket.header,parsedPacket.payload);
		}
		if (payloadid == 0x0191)
		{
			//m_lastDatalogUpdateEnabled = true;
			//m_lastDatalogTime = QDateTime::currentMSecsSinceEpoch();

			//Need to pull sequence number out of here
		}
		else
		{
			if (parsedPacket.isNAK)
			{
				unsigned short errornum = ((unsigned char)parsedPacket.payload[0]) << 8;
				errornum += ((unsigned char)parsedPacket.payload[1]);
				emit packetNaked(payloadid,parsedPacket.header,parsedPacket.payload,errornum);
				return;
			}
			if (parsedPacket.isPartial)
			{
				qDebug() << "Partial packet";
				emit partialPacketAcked(payloadid,parsedPacket.header,parsedPacket.payload);
			}
			else if (parsedPacket.isComplete)
			{
				qDebug() << "Completed packet";
				emit completePacketAcked(payloadid,parsedPacket.header,parsedPacket.payload);
			}
			else
			{
				emit packetAcked(payloadid,parsedPacket.header,parsedPacket.payload);
			}

		}


	}
	else
	{
		QLOG_FATAL() << "Header size is only" << parsedPacket.header.length() << "! THIS SHOULD NOT HAPPEN!";
		QString headerstring = "";
		QString packetstring = "";
		for (int i=0;i<parsedPacket.header.size();i++)
		{
			headerstring += QString::number((unsigned char)parsedPacket.header[i],16);
		}
		for (int i=0;i<parsedPacket.payload.size();i++)
		{
			packetstring += QString::number((unsigned char)parsedPacket.payload[i],16);
		}
		QLOG_DEBUG() << "Header:" << headerstring;
		QLOG_DEBUG() << "Packet:" << packetstring;
	}
}
