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

#include "../plugins/libreems/protocoldecoder.h"
#include <QDebug>
#include "QsLog.h"
ProtocolDecoder::ProtocolDecoder(QObject *parent) :
	QObject(parent)
{
	m_isInPacket = false;
	m_isInEscape = false;
}
void ProtocolDecoder::parseBuffer(QByteArray buffer)
{
	return;
	for (int i=0;i<buffer.size();i++)
	{
		if (((unsigned char)buffer.at(i)) == static_cast<unsigned char>(0xAA))
		{
			if (m_isInPacket)
			{
				//Bad start
				QLOG_DEBUG() << "Bad Start";
			}
			m_isInPacket = true;
			m_isInEscape = false;
			m_currMsg.clear();
		}
		else if (static_cast<unsigned char>(buffer.at(i)) == static_cast<unsigned char>(0xCC))
		{
			if (!m_isInPacket)
			{
				//Bad stop
				QLOG_DEBUG() << "Bad Stop";
				continue;
			}
			m_isInPacket = false;
			//m_currMsg; //Current packet.
			QByteArray toemit = m_currMsg;
			toemit.detach();
			if (!toemit.startsWith(QByteArray().append(0x01).append(0x01).append(0x91)))
			{
				QLOG_DEBUG() << "protocol decoder incoming packet";
			}
			else
			{
				//QLOG_DEBUG() << "protocol decoder DATALOG PACKET";
			}
			//QLOG_DEBUG() << "MSG:" << toemit.toHex();
			emit newPacket(toemit);
			m_currMsg.clear();
		}
		else if (m_isInPacket)
		{
			if (m_isInEscape)
			{
				if (static_cast<unsigned char>(buffer.at(i)) == 0x55)
				{
					m_currMsg.append(0xAA);
				}
				else if (static_cast<unsigned char>(buffer.at(i)) == 0x44)
				{
					m_currMsg.append(0xBB);
				}
				else if (static_cast<unsigned char>(buffer.at(i)) == 0x33)
				{
					m_currMsg.append(0xCC);
				}
				else
				{
					//Bad escape character
					QLOG_DEBUG() << "Bad Escape char";
				}
				m_isInEscape = false;
			}
			else if (static_cast<unsigned char>(buffer.at(i)) == 0xBB)
			{
				m_isInEscape = true;
			}
			else
			{
				m_currMsg.append(buffer.at(i));
			}
		}
		else
		{
			QLOG_DEBUG() << "Out of packet bytes" << QString::number(buffer.at(i),16);
			//Out of packet bytes.
		}
	}
}
