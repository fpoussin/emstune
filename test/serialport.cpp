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

#include "../plugins/libreems/serialport.h"

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
void SerialPort::connectToPort(QString portname)
{
    start();
}
void SerialPort::run()
{
    emit connected();
    while (true) {

        msleep(10);
    }
}

void SerialPort::setInterByteSendDelay(int milliseconds)
{
    m_interByteSendDelay = milliseconds;
}

int SerialPort::writeBytes(QByteArray packet)
{
    return packet.size();
}

void SerialPort::closePort()
{
    QLOG_DEBUG() << "SerialPort::closePort thread:" << QThread::currentThread();
    //m_serialPort->close();
    //delete m_serialPort;
    m_privBuffer.clear();
}
