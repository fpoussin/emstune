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

#include "../plugins/libreems/protocolencoder.h"
#include "testcontrol.h"
#include "../plugins/libreems/packet.h"
extern TestControl *m_testControl;
ProtocolEncoder::ProtocolEncoder()
{
}
QByteArray ProtocolEncoder::encodePacket(unsigned short payloadid, QList<QVariant> arglist, QList<int> argsizelist, bool haslength)
{
    if (payloadid == GET_FIRMWARE_VERSION) {
        m_testControl->firmwareMessage();
    } else if (payloadid == GET_INTERFACE_VERSION) {
        m_testControl->getInterfaceVersion();
    } else if (payloadid == GET_COMPILER_VERSION) {
        m_testControl->getCompilerVersion();
    } else if (payloadid == GET_DECODER_NAME) {
        m_testControl->getDecoderName();
    } else if (payloadid == GET_FIRMWARE_BUILD_DATE) {
        m_testControl->getFirmwareBuildDate();
    } else if (payloadid == GET_MAX_PACKET_SIZE) {
        m_testControl->getMaxPacketSize();
    } else if (payloadid == GET_OPERATING_SYSTEM) {
        m_testControl->getOperatingSystem();
    } else if (payloadid == GET_DATALOG_DESCRIPTOR) {
        m_testControl->getDatalogDescriptor();
    } else if (payloadid == GET_FIELD_DESCRIPTOR) {
    } else if (payloadid == RETRIEVE_JSON_TABLE_DESCRIPTOR) {

    } else if (payloadid == GET_LOCATION_ID_LIST) {
        m_testControl->getLocationIdList();
    } else if (payloadid == GET_LOCATION_ID_INFO) {
        m_testControl->getLocationIdInfo(arglist.at(0).toInt());
    } else if (payloadid == RETRIEVE_BLOCK_IN_RAM) {
        m_testControl->getBlockInRam(arglist.at(0).toInt(), arglist.at(1).toInt(), arglist.at(2).toInt());
    } else if (payloadid == UPDATE_BLOCK_IN_RAM) {
        m_testControl->updateBlockInRam(arglist.at(0).toInt(), arglist.at(1).toInt(), arglist.at(2).toInt(), arglist.at(3).toByteArray());
    }
    return QByteArray().append((char)0x00).append((char)0x01);
    if (arglist.size() != argsizelist.size()) {
        return QByteArray();
    }
    QByteArray header;
    QByteArray payload;
    for (int i = 0; i < arglist.size(); i++) {
        if (arglist[i].type() == QVariant::Int) {
            if (argsizelist[i] == 1) {
                unsigned char arg = arglist[i].toInt();
                payload.append((unsigned char)((arg)&0xFF));
            } else if (argsizelist[i] == 2) {
                unsigned short arg = arglist[i].toInt();
                payload.append((unsigned char)((arg >> 8) & 0xFF));
                payload.append((unsigned char)((arg)&0xFF));
            } else if (argsizelist[i] == 4) {
                unsigned int arg = arglist[i].toInt();
                payload.append((unsigned char)((arg >> 24) & 0xFF));
                payload.append((unsigned char)((arg >> 16) & 0xFF));
                payload.append((unsigned char)((arg >> 8) & 0xFF));
                payload.append((unsigned char)((arg)&0xFF));
            }
        } else if (arglist[i].type() == QVariant::ByteArray) {
            //Data packet
            QByteArray arg = arglist[i].toByteArray();
            payload.append(arg);
        } else if (arglist[i].type() == QVariant::String) {
            QByteArray arg = arglist[i].toString().toLatin1();
            payload.append(arg);
        }
    }
    if (haslength) {
        header.append((unsigned char)0x01); //Length, no seq no nak
        header.append((unsigned char)((payloadid >> 8) & 0xFF));
        header.append((unsigned char)((payloadid)&0xFF));
        header.append((char)(payload.length() >> 8) & 0xFF);
        header.append((char)(payload.length()) & 0xFF);
    } else {
        header.append((char)0x00); //No Length, no seq no nak
        header.append((char)((payloadid >> 8) & 0xFF));
        header.append((char)((payloadid)&0xFF));
    }
    return generatePacket(header, payload);
}
QByteArray ProtocolEncoder::generatePacket(QByteArray header, QByteArray payload)
{
    QByteArray packet;
    packet.append((char)0xAA);
    unsigned char checksum = 0;
    for (int i = 0; i < header.size(); i++) {
        checksum += header[i];
    }
    for (int i = 0; i < payload.size(); i++) {
        checksum += payload[i];
    }
    payload.append(checksum);
    for (int j = 0; j < header.size(); j++) {
        if (header[j] == (char)0xAA) {
            packet.append((char)0xBB);
            packet.append((char)0x55);
        } else if (header[j] == (char)0xBB) {
            packet.append((char)0xBB);
            packet.append((char)0x44);
        } else if (header[j] == (char)0xCC) {
            packet.append((char)0xBB);
            packet.append((char)0x33);
        } else {
            packet.append(header[j]);
        }
    }
    for (int j = 0; j < payload.size(); j++) {
        if (payload[j] == (char)0xAA) {
            packet.append((char)0xBB);
            packet.append((char)0x55);
        } else if (payload[j] == (char)0xBB) {
            packet.append((char)0xBB);
            packet.append((char)0x44);
        } else if (payload[j] == (char)0xCC) {
            packet.append((char)0xBB);
            packet.append((char)0x33);
        } else {
            packet.append(payload[j]);
        }
    }
    packet.append((char)0xCC);
    return packet;
}
