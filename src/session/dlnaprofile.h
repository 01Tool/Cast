#pragma once

#include "capture/displaysource.h"
#include "session/wfdaudiomode.h"
#include "session/wfdvideomode.h"

#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QUrl>

struct DlnaProfile {
    QString mime = QStringLiteral("video/mpeg");
    QString protocolInfo;
    QString contentFeatures;
};

QString xmlEscape(const QString &text);
QString parseConnectionManagerSink(const QByteArray &soapXml);
DlnaProfile pickDlnaProfile(const QString &sinkProtocolInfo);
QString buildDidlLite(const QUrl &uri, const DlnaProfile &profile, const QString &title);
QByteArray buildSoapEnvelope(const QString &serviceType, const QString &action,
                             const QString &innerXml);
QHostAddress pickLocalIpv4(const QHostAddress &peer);
WfdVideoMode dlnaVideoMode(const DisplaySource &source);
WfdAudioMode dlnaAudioMode(bool enabled);