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

SerialPort::SerialPort(QObject *parent) : QThread(parent)
{
	m_serialLockMutex = new QMutex();
	m_interByteSendDelay=0;
	m_inpacket = false;
	m_inescape = false;
	m_packetErrorCount=0;

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
	try
	{
		serialPort = new serial::Serial();
		serialPort->setPort(m_portName.toStdString());
		serialPort->open();
		if (!serialPort->isOpen())
		{
			qDebug() << "Error opening port:";
			emit unableToConnect("Error opening port");
			return;
		}
		serialPort->setBaudrate(115200);
		serialPort->setParity(serial::parity_odd);
		serialPort->setStopbits(serial::stopbits_one);
		serialPort->setBytesize(serial::eightbits);
		serialPort->setFlowcontrol(serial::flowcontrol_none);
		serialPort->setTimeout(1,1,0,100,0); //1ms read timeout, 100ms write timeout.

		emit connected();
	}
	catch (serial::IOException ex)
	{
		emit unableToConnect(ex.what());
		return;
	}
	catch (std::exception ex)
	{
		emit unableToConnect(ex.what());
		return;
	}
	uint8_t buffer[1024];
	bool isopen = true;
	while(isopen)
	{
		try
		{
			m_serialLockMutex->lock();
			isopen = serialPort->isOpen();
			if (serialPort->available())
			{
				int size = serialPort->read(buffer,1024);
				emit bytesReady(QByteArray((const char*)buffer,size));
				m_serialLockMutex->unlock();
			}
			else
			{
				m_serialLockMutex->unlock();
				msleep(10);
			}

		}
		catch (serial::IOException ex)
		{
			//Error here
			serialPort->close();
			delete serialPort;
			emit error(ex.what());
			emit disconnected();
			return;
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
	serialPort->write((const uint8_t*)packet.data(),packet.length());
	QLOG_TRACE() << "Wrote bytes";
	return packet.size();

}

void SerialPort::closePort()
{
	m_serialLockMutex->lock();
	QLOG_DEBUG() << "SerialPort::closePort thread:" << QThread::currentThread();
	//m_serialPort->close();
	serialPort->close();
	//delete m_serialPort;
	m_privBuffer.clear();
	m_serialLockMutex->unlock();
}
