#pragma once

#include <QString>
#include <QUrl>

enum class CastProtocol {
    Miracast,
    Dlna,
};

// Hint from ConnectionManager Sink ProtocolInfo. Not a measured verdict.
enum class DlnaMediaKind {
    Unknown,
    LiveTsLikely,
    FileOnlyLikely,
    NoVideo,
};

inline QString dlnaMediaKindKey(DlnaMediaKind kind)
{
    switch (kind) {
    case DlnaMediaKind::LiveTsLikely:
        return QStringLiteral("live-ts-likely");
    case DlnaMediaKind::FileOnlyLikely:
        return QStringLiteral("file-only-likely");
    case DlnaMediaKind::NoVideo:
        return QStringLiteral("no-video");
    case DlnaMediaKind::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

struct SinkDevice
{
    QString id;
    QString name;
    QString address;
    CastProtocol protocol = CastProtocol::Miracast;
    QString p2pDevicePath;
    QString p2pMac;
    QString miceHost;
    bool wfdCapable = false;
    bool miceCapable = false;
    QUrl locationUrl;
    QString udn;
    QUrl avTransportUrl;
    QUrl connectionManagerUrl;
    QString protocolInfo;
    DlnaMediaKind dlnaMedia = DlnaMediaKind::Unknown;
    QString dlnaMediaSummary;
};
