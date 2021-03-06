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

#ifndef FREEEMSCOMMS_H
#define FREEEMSCOMMS_H

#include <QThread>
#include <QFile>
#include <QMutex>
#include <QVariant>
#include <QDateTime>
#include <QTimer>
#include "serialport.h"
#include "emscomms.h"
#include "fedatapacketdecoder.h"
#include "fememorymetadata.h"
#include "table3ddata.h"
#include "table2ddata.h"
#include "emsdata.h"
#include "ferawdata.h"
#include "packet.h"
#include "packetdecoder.h"
#include "protocolencoder.h"
#include "protocoldecoder.h"
#include "feconfigdata.h"

class FreeEmsComms : public EmsComms
{
	Q_OBJECT
	Q_INTERFACES(EmsComms)
	Q_PLUGIN_METADATA(IID "EmsComms/1.0")
public:
	QMutex m_serialPortMutex;
	FreeEmsComms(QObject *parent = 0);
	~FreeEmsComms();
	QString getPluginCompat() { return QString("FREEEMS"); }
	DataPacketDecoder *getDecoder();
	MemoryMetaData *getMetaParser();
	Table3DData *getNew3DTableData();
	Table2DData *getNew2DTableData();
	void passLogger(QsLogging::Logger *log);
	void stop() { m_terminateLoop = true; }
	void setLogsEnabled(bool enabled);
	void setLogDirectory(QString dir);
	void setPort(QString portname);
	void setBaud(int baudrate);
	void setLogFileName(QString filename);
    bool sendPacket(unsigned short payloadid);
    bool sendPacket(unsigned short payloadid,QList<QVariant> arglist,QList<int> argsizelist=QList<int>(),bool haslength=false);
	int getLocationIdInfo(unsigned short locationid);
	int getInterfaceVersion();
	int getFirmwareVersion();
	int getMaxPacketSize();
	int getDecoderName();
	int getFirmwareBuildDate();
	int getCompilerVersion();
	int getSupportEmail();
	int getBuiltByName();
	int getOperatingSystem();
	int getDatalogDescriptor();
	int getFieldDescriptor();
	int getTableDescriptor();
	int echoPacket(QByteArray packet);
	Q_INVOKABLE int startBenchTest(unsigned char eventspercycle,unsigned short numcycles,unsigned short ticksperevent,QVariantList pineventmask,QVariantList pinmode);
	Q_INVOKABLE int stopBenchTest();
	Q_INVOKABLE int bumpBenchTest(unsigned char cyclenum);
	int getLocationIdList(unsigned char listtype, unsigned short listmask);
	int softReset();
	int hardReset();
	bool sendSimplePacket(unsigned short payload);
	void connectSerial(QString port,int baud);
	void disconnectSerial();
	void loadLog(QString filename);
	void playLog();
	void startInterrogation();
	void setInterByteSendDelay(int milliseconds);
	void setlogsDebugEnabled(bool enabled);
	void verifyRamFromDevice(quint64 checksum);
	int enableDatalogStream();
	int disableDatalogStream();
	Table2DData* get2DTableData(unsigned short locationid);
	Table3DData* get3DTableData(unsigned short locationid);
	Table2DData* get2DTableData(QString locationname);
	Table3DData* get3DTableData(QString locationname);
	RawData* getRawData(unsigned short locationid);
	ConfigData* getConfigData(QString name);
	QList<QString> getConfigList();
	void writeAllRamToRam();
	void acceptLocalChanges();
	void rejectLocalChanges();

protected:
	void run();
private:

	int m_state;


	class RequestClass
	{
	public:
		RequestClass()
		{
			retryCount=0;
			hasLength = false;
			hasReply = false;
			sentRequest = false;
		}
		bool sentRequest; //If the request can be directly passed to attemptSendPacket to be sent to the ECU.

		unsigned char retryCount;
		bool hasLength;
		bool hasReply;
		RequestType type;
		QList<QVariant> args;
		QList<int> argsize;
		int sequencenumber;
		void addArg(QVariant arg,int size=0) { args.append(arg); argsize.append(size);}
	};
	ProtocolEncoder m_protocolEncoder;
    bool m_firstPacketValid;
	bool m_isSilent;
	quint64 m_lastDatalogTime;
	qint64 m_lastMessageSentTime;
	QTimer *m_lastdatalogTimer;
	QTimer *m_sentMessageTimeoutTimer;
	bool m_lastDatalogUpdateEnabled;
	bool m_isConnected;
	FEDataPacketDecoder *dataPacketDecoder;
	FEMemoryMetaData *m_metaDataParser;
	bool m_debugLogsEnabled;
	QMap<LocationIdFlags,QString> m_blockFlagToNameMap;
	bool m_terminateLoop;
	QMutex m_waitingInfoMutex;
	bool sendPacket(RequestClass request);
	qint64 m_timeoutMsecs;
	QList<LocationIdFlags> m_blockFlagList;
	int m_sequenceNumber;
	QMutex m_reqListMutex;
	QList<RequestClass> m_reqList;
	QList<RequestClass> m_threadReqList;
	SerialPort *serialPort;
	bool m_waitingForResponse;
	bool m_logsEnabled;
	QString m_logsDirectory;
	QString m_logsFilename;
	unsigned int m_payloadWaitingForResponse;
	RequestClass m_currentWaitingRequest;
	QFile *m_logInFile;
	QFile *m_logOutFile;
	QFile *m_logInOutFile;
	void openLogs();
	bool m_interrogateInProgress;
	bool m_interrogateIdListComplete;
	bool m_interrogateIdInfoComplete;
	bool m_interogateComplete;
	bool m_waitingForRamWrite;
	bool m_waitingForFlashWrite;
	QList<int> m_interrogatePacketList;
	int m_interrogateTotalCount;
	QList<unsigned short> m_locationIdList;
	EmsData emsData;
	QMap<unsigned short,Table2DData*> m_2dTableMap;
	QMap<unsigned short,Table3DData*> m_3dTableMap;
	QMap<unsigned short,RawData*> m_rawDataMap;
	QMap<unsigned short,QList<FEConfigData*> > m_locIdToConfigListMap;
	QMap<QString,FEConfigData*> m_configMap;
	PacketDecoder *m_packetDecoder;
	ProtocolDecoder *m_protocolDecoder;
	QThread *m_packetProcessingThread;
	QMap<QString,QString> m_interrogationMetaDataMap;
	void sendNextInterrogationPacket();
	QList<unsigned int> m_dirtyRamAddresses;
	QMap<int,TableMeta> m_tableMetaMap;
	QMap<int,FieldMeta> m_fieldMetaMap;
signals:
	void packetSent(unsigned short locationid,QByteArray header,QByteArray payload);
	void packetAcked(unsigned short locationid,QByteArray header,QByteArray payload);
	void packetNaked(unsigned short locationid,QByteArray header,QByteArray payload,unsigned short errornum);
	void decoderFailure(QByteArray packet);
	void connected();
	void disconnected();
	void locationIdList(QList<unsigned short> idlist);
	void locationIdInfo(unsigned short locationid,MemoryLocationInfo info);
	void ramBlockRetrieved(unsigned short locationid, QByteArray header,QByteArray payload);
	void flashBlockRetrieved(unsigned short locationid,QByteArray header,QByteArray payload);
	void dataLogPayloadReceived(QByteArray header,QByteArray payload);
	void debugVerbose(QString msg);
	void unknownPacket(QByteArray header,QByteArray payload);
	void debug(QString msg);
	void error(QString msg);
	void error(SerialPortStatus error,QString msg = QString());
	void commandSuccessful(int sequencenumber);
	void commandFailed(int sequencenumber,unsigned short errornum);
	void commandTimedOut(int sequencenumber);
	void emsSilenceStarted(qint64 lastdatalogtime);
	void emsSilenceBroken();
	void interrogateTaskStart(QString task, int sequence);
	void interrogateTaskSucceed(int sequence);
	void interrogateTaskFail(int sequence);
	void interrogationProgress(int current, int total);
	void interrogationComplete();
	void deviceDataUpdated(unsigned short locationid);
	void interrogationData(QMap<QString,QString> datamap);
	void dataLogPayloadDecoded(QVariantMap data);
	void resetDetected(int missedPackets);
	void configRecieved(ConfigBlock,QVariant);
	void memoryDirty();
	void memoryClean();
	void benchTestReply(unsigned short countRemaining,unsigned char currentEvent);
	void flashLocationDirty(unsigned short locationid);
	void ramLocationDirty(unsigned short locationid);
	void datalogDescriptor(QString data);
	void firmwareDebugReceived(QString text);
	void deviceRamLocationDirty(unsigned short locationid);
	void deviceFlashLocationNotSynced(unsigned short locationid);
public slots:
	int updateBlockInRam(unsigned short location,unsigned short offset, unsigned short size,QByteArray data);
	int updateBlockInFlash(unsigned short location,unsigned short offset, unsigned short size,QByteArray data);
	int retrieveBlockFromRam(unsigned short location, unsigned short offset, unsigned short size,bool mark=true);
	int retrieveBlockFromFlash(unsigned short location, unsigned short offset, unsigned short size,bool mark=true);
	int burnBlockFromRamToFlash(unsigned short location,unsigned short offset, unsigned short size);
private slots:
	void serialPortConnected();
	void serialPortUnableToConnect(QString error);
	void serialPortDisconnected();
	void serialPortError(QString errorstr);
	void triggerNextSend();
	void dataLogPayloadReceivedRec(QByteArray header,QByteArray payload);
	void ramBlockUpdateRec(QByteArray header,QByteArray payload);
	void rxThreadPortGone();
	void ramLocationMarkedDirty(unsigned short locationid);
	void flashLocationMarkedDirty(unsigned short locationid);
	void flashBlockUpdateRec(QByteArray header,QByteArray payload);
	void packetNakedRec(unsigned short payloadid,QByteArray header,QByteArray payload,unsigned short errornum);
	void packetAckedRec(unsigned short payloadid,QByteArray header,QByteArray payload);
	void partialPacketAckedRec(unsigned short payloadid,QByteArray header,QByteArray payload);
	void completePacketAckedRec(unsigned short payloadid,QByteArray header,QByteArray payload);
	void locationIdInfoRec(MemoryLocationInfo info);
	void locationIdListRec(QList<unsigned short> locationidlist);
	void datalogTimerTimeout();
	void dataLogWrite(QByteArray buffer);
	void dataLogRead(QByteArray buffer);
	void parseEverything(QByteArray buffer);
	//Packet parseBuffer(QByteArray buffer);
	void locationIdUpdate(unsigned short locationid);
	void copyFlashToRam(unsigned short locationid);
	void copyRamToFlash(unsigned short locationid);
	void decoderName(QString name);
	void firmwareBuild(QString date);
	void compilerVersion(QString version);
	void operatingSystem(QString os);
	void interfaceVersion(QString version);
	void firmwareVersion(QString version);
	void builtByName(QString name);
	void supportEmail(QString email);
	void timeoutTimerTick();
	void saveDatalogDescriptor(QString json);
	void fieldDescriptor(QString json);
	void tableDescriptor(QString json);
	void firmwareDebug(QString text);
	void deviceRamLocationMarkedDirty(unsigned short locationid);


};

#endif // FREEEMSCOMMS_H
