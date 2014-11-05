#include "protocoldecoder.h"
#include <QDebug>
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
		if (((unsigned char)buffer.at(i)) == static_cast<unsigned char>(0xAA))
		{
			if (m_isInPacket)
			{
				//Bad start
				qDebug() << "Bad Start";
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
				qDebug() << "Bad Stop";
				continue;
			}
			m_isInPacket = false;
			m_currMsg; //Current packet.
			QByteArray toemit(m_currMsg);
			toemit.detach();
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
					qDebug() << "Bad Escape char";
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
			//qDebug() << "Out of packet bytes" << QString::number(buffer.at(i),16);
			//Out of packet bytes.
		}
	}
}
