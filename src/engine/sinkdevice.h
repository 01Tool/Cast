#pragma once

#include <QString>
#include <QUrl>

enum class CastProtocol {
    Miracast,
    Dlna,
};

struct SinkDevice
{
    QString id;
    QString name;
    QString address;
    CastProtocol protocol = CastProtocol::Miracast;
    QString p2pDevicePath;
    bool wfdCapable = false;
    QUrl locationUrl;
    QString udn;
    QUrl avTransportUrl;
    QUrl connectionManagerUrl;
    QString protocolInfo;
};
