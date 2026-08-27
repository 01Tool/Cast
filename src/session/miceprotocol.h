#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

constexpr quint16 kMicePort = 7250;
constexpr quint16 kWfdRtspPort = 7236;
constexpr quint8 kMiceVersion = 0x01;
constexpr quint8 kMiceCmdSourceReady = 0x01;
constexpr quint8 kMiceCmdStopProjection = 0x02;
constexpr int kMiceSourceIdSize = 16;
constexpr int kMiceFriendlyNameMaxBytes = 520;

struct MiceArpRow {
    QString ip;
    QString mac;
    quint32 flags = 0;
};

struct MiceDnsService {
    QString name;
    QString host;
    QString ipv4;
    QString p2pMac;
    quint16 port = kMicePort;
};

QByteArray miceDefaultSourceId();
QByteArray encodeUtf16Le(const QString &text);
QByteArray encodeMiceSourceReady(const QString &friendlyName, const QByteArray &sourceId,
                                 quint16 rtspPort = kWfdRtspPort);
QByteArray encodeMiceStopProjection(const QString &friendlyName, const QByteArray &sourceId);

QString normalizeMac(const QString &mac);
QString toggleMacLaa(const QString &mac);
bool macsRelated(const QString &a, const QString &b);

QVector<MiceArpRow> parseArpTable(const QByteArray &text);
QString ipv4ForHardwareAddress(const QString &mac, const QByteArray &arpText);

QByteArray buildMdnsDisplayPtrQuery();
QVector<MiceDnsService> parseMdnsDisplayServices(const QByteArray &packet,
                                                 const QString &senderIpv4 = {});
