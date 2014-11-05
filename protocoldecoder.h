#ifndef PROTOCOLDECODER_H
#define PROTOCOLDECODER_H

#include <QObject>

class ProtocolDecoder : public QObject
{
	Q_OBJECT
public:
	explicit ProtocolDecoder(QObject *parent = 0);
private:
	QByteArray m_currMsg;
	bool m_isInPacket;
	bool m_isInEscape;
signals:
	void newPacket(QByteArray packet);
public slots:
	void parseBuffer(QByteArray buffer);
};

#endif // PROTOCOLDECODER_H
