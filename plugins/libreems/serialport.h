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

#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QThread>
#include <QFile>
#include <QDebug>
#include <QMutex>
#include <qglobal.h>
#include <vector>
#include "serialportstatus.h"
#include "memorylocationinfo.h"
#include "protocoldecoder.h"
#include <QtSerialPort>

class SerialPort : public QThread
{
    Q_OBJECT
public:
    SerialPort(QObject *parent = 0);
    void setPort(QString portname);
    void setBaud(int baudrate);
    void closePort();
    int writeBytes(QByteArray bytes);
    int bufferSize() { return m_queuedMessages.size(); }
    void setInterByteSendDelay(int milliseconds);
    QString portName() { return m_portName; }
    ProtocolDecoder *m_protocolDecoder;
public slots:
    void connectToPort(QString portname);

private:
    void run();
    void openLogs();

    QSerialPort *m_serialPort;
    QSerialPortInfo *m_serialPortInfo;
    QByteArray m_privBuffer;
    QMutex *m_serialLockMutex;
    unsigned int m_packetErrorCount;
    bool m_logsEnabled;
    QString m_logDirectory;
    bool m_inpacket;
    bool m_inescape;
    int m_interByteSendDelay;
    QList<QByteArray> m_queuedMessages;
    QByteArray m_buffer;
    QString m_logFileName;
    QString m_portName;
    int m_baud;
signals:
    void packetReceived(QByteArray packet);
    void parseBuffer(QByteArray buffer);
    void dataWritten(QByteArray data);
    void bytesReady(QByteArray buffer);
    void connected();
    void unableToConnect(QString error);
    void disconnected();
    void error(QString error);
};

#endif // SERIALPORT_H
