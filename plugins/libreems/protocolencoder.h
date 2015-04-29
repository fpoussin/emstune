#ifndef PROTOCOLENCODER_H
#define PROTOCOLENCODER_H
#include <QByteArray>
#include <QList>
#include <QVariant>
class ProtocolEncoder
{
public:
	ProtocolEncoder();
	QByteArray encodePacket(unsigned short payloadid,QList<QVariant> arglist,QList<int> argsizelist,bool haslength);
private:
	QByteArray generatePacket(QByteArray header,QByteArray payload);
};

#endif // PROTOCOLENCODER_H
