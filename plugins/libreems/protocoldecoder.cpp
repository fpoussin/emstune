#include "protocoldecoder.h"
#include <QDebug>
#include "QsLog.h"

#define START_BYTE static_cast<unsigned char>(0xAA)
#define ESCAPE_BYTE static_cast<unsigned char>(0xBB)
#define STOP_BYTE static_cast<unsigned char>(0xCC)

#define ESCAPE_START static_cast<unsigned char>(0x55)
#define ESCAPE_ESCAPE static_cast<unsigned char>(0x44)
#define ESCAPE_STOP static_cast<unsigned char>(0x33)

ProtocolDecoder::ProtocolDecoder(QObject *parent) :
	QObject(parent)
{
	m_isInPacket = false;
	m_isInEscape = false;
}
void ProtocolDecoder::parseBuffer(QByteArray buffer)
{
	for (int i=0;i<buffer.size();i++)
	{
		if (static_cast<unsigned char>(buffer.at(i)) == START_BYTE)
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
		else if (static_cast<unsigned char>(buffer.at(i)) == STOP_BYTE)
		{
			if (!m_isInPacket)
			{
				//Bad stop
				QLOG_DEBUG() << "Bad Stop";
				continue;
			}
			m_isInPacket = false;
			QByteArray toemit = m_currMsg;
			toemit.detach();
			emit newPacket(toemit);
			m_currMsg.clear();
		}
		else if (m_isInPacket)
		{
			if (m_isInEscape)
			{
				if (static_cast<unsigned char>(buffer.at(i)) == ESCAPE_START)
				{
					m_currMsg.append(START_BYTE);
				}
				else if (static_cast<unsigned char>(buffer.at(i)) == ESCAPE_ESCAPE)
				{
					m_currMsg.append(ESCAPE_BYTE);
				}
				else if (static_cast<unsigned char>(buffer.at(i)) == ESCAPE_STOP)
				{
					m_currMsg.append(STOP_BYTE);
				}
				else
				{
					//Bad escape character
					QLOG_DEBUG() << "Bad Escape char";
				}
				m_isInEscape = false;
			}
			else if (static_cast<unsigned char>(buffer.at(i)) == ESCAPE_BYTE)
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
			//Out of packet bytes.
			QLOG_DEBUG() << "Out of packet bytes" << QString::number(buffer.at(i),16);
		}
	}
}
