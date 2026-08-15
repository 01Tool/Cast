#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QUrl>

struct SsdpResponse {
    QUrl location;
    QString usn;
    QString st;
    QString server;
    QHostAddress peer;
};

QByteArray buildSsdpMsearch(int mxSeconds = 3);

// Unicast M-SEARCH reply or multicast NOTIFY. Empty location → invalid.
SsdpResponse parseSsdpDatagram(const QByteArray &datagram);