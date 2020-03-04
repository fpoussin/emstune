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

#include "serialport.h"
#include <QDateTime>
#include <QMutexLocker>
#include <cassert>
#include "QsLog.h"

SerialPort::SerialPort(QObject *parent)
    : QThread(parent)
{
    m_serialLockMutex = new QMutex();
    m_interByteSendDelay = 0;
    m_inpacket = false;
    m_inescape = false;
    m_packetErrorCount = 0;

    //	connect(m_protocolDecoder,SIGNAL(newPacket(QByteArray)),this,SIGNAL(packetReceived(QByteArray)));
}
void SerialPort::setPort(QString portname)
{
    m_portName = portname;
}
void SerialPort::setBaud(int baudrate)
{
    m_baud = baudrate;
}
void SerialPort::run()
{
    m_serialPort = new QSerialPort(m_portName, this);
    m_serialPort->open(QIODevice::ReadWrite);

    if (!m_serialPort->isOpen()) {
        qDebug() << "Error opening port:";
        emit unableToConnect("Error opening port");
        return;
    }

    m_serialPort->setBaudRate(115200);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    emit connected();

    bool isopen = true;
    while (isopen && !this->isInterruptionRequested()) {
        m_serialLockMutex->lock();
        isopen = m_serialPort->isOpen();
        m_serialPort->waitForReadyRead(10);
        if (m_serialPort->readBufferSize() > 0) {
            QByteArray buffer = m_serialPort->readAll();
            int size = buffer.size();
            emit bytesReady(QByteArray((const char *)buffer, size));
            m_serialLockMutex->unlock();
        } else {
            m_serialLockMutex->unlock();
        }
    }
}

void SerialPort::connectToPort(QString portname)
{
    m_portName = portname;
    start();
}

void SerialPort::setInterByteSendDelay(int milliseconds)
{
    m_interByteSendDelay = milliseconds;
}

int SerialPort::writeBytes(QByteArray packet)
{
    QLOG_TRACE() << "Writing bytes:" << packet.size();
    m_serialPort->write(packet);
    m_serialPort->waitForBytesWritten(100);
    QLOG_TRACE() << "Wrote bytes";
    return packet.size();
}

void SerialPort::closePort()
{
    m_serialLockMutex->lock();
    QLOG_DEBUG() << "SerialPort::closePort thread:" << QThread::currentThread();
    m_serialPort->close();
    delete m_serialPort;
    m_privBuffer.clear();
    m_serialLockMutex->unlock();
}
