#include "session/dlnaprofile.h"

#include <QNetworkInterface>
#include <QXmlStreamReader>

namespace {

constexpr auto kDefaultFeatures =
    "DLNA.ORG_PN=MPEG_TS_SD_NA_ISO;DLNA.ORG_OP=00;DLNA.ORG_CI=0;"
    "DLNA.ORG_FLAGS=01700000000000000000000000000000";

int even(int value)
{
    return (value / 2) * 2;
}

bool mimeLooksLikeMpegTs(const QString &mime)
{
    return mime.contains(QLatin1String("video/mpeg"), Qt::CaseInsensitive)
        || mime.contains(QLatin1String("video/vnd.dlna.mpeg-tts"), Qt::CaseInsensitive)
        || mime.contains(QLatin1String("video/mp2t"), Qt::CaseInsensitive)
        || mime.contains(QLatin1String("video/x-mpegts"), Qt::CaseInsensitive);
}

QString fieldAfterColons(const QString &protocolInfo, int index)
{
    const QStringList parts = protocolInfo.split(QLatin1Char(':'));
    if (parts.size() <= index)
        return {};
    return parts.at(index);
}

bool isUsableIpv4(const QHostAddress &ip)
{
    if (ip.protocol() != QAbstractSocket::IPv4Protocol)
        return false;
    if (ip.isLoopback() || ip.isLinkLocal() || ip.isNull())
        return false;
    return true;
}

bool looksLikeP2pInterface(const QNetworkInterface &iface)
{
    const QString name = iface.name();
    return name.startsWith(QLatin1String("p2p")) || name.startsWith(QLatin1String("wifi-p2p"));
}

} // namespace

QString xmlEscape(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (const QChar ch : text) {
        switch (ch.unicode()) {
        case '&':
            out += QLatin1String("&amp;");
            break;
        case '<':
            out += QLatin1String("&lt;");
            break;
        case '>':
            out += QLatin1String("&gt;");
            break;
        case '"':
            out += QLatin1String("&quot;");
            break;
        case '\'':
            out += QLatin1String("&apos;");
            break;
        default:
            out += ch;
            break;
        }
    }
    return out;
}

QString parseConnectionManagerSink(const QByteArray &soapXml)
{
    QXmlStreamReader xml(soapXml);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement())
            continue;
        if (xml.name().toString() == QLatin1String("Sink"))
            return xml.readElementText(QXmlStreamReader::SkipChildElements).trimmed();
    }
    return {};
}

DlnaProfile pickDlnaProfile(const QString &sinkProtocolInfo)
{
    DlnaProfile profile;
    profile.contentFeatures = QString::fromLatin1(kDefaultFeatures);
    profile.protocolInfo = QStringLiteral("http-get:*:video/mpeg:%1").arg(profile.contentFeatures);

    const QStringList entries = sinkProtocolInfo.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (QString entry : entries) {
        entry = entry.trimmed();
        if (!entry.startsWith(QLatin1String("http-get:"), Qt::CaseInsensitive))
            continue;
        const QString mime = fieldAfterColons(entry, 2);
        if (!mimeLooksLikeMpegTs(mime))
            continue;
        profile.mime = mime;
        QString extra = fieldAfterColons(entry, 3);
        if (extra.isEmpty() || extra == QLatin1String("*"))
            extra = QString::fromLatin1(kDefaultFeatures);
        extra.replace(QLatin1String("DLNA.ORG_OP=01"), QLatin1String("DLNA.ORG_OP=00"));
        extra.replace(QLatin1String("DLNA.ORG_OP=10"), QLatin1String("DLNA.ORG_OP=00"));
        extra.replace(QLatin1String("DLNA.ORG_OP=11"), QLatin1String("DLNA.ORG_OP=00"));
        profile.contentFeatures = extra;
        profile.protocolInfo = QStringLiteral("http-get:*:%1:%2").arg(mime, extra);
        return profile;
    }
    return profile;
}

QString buildDidlLite(const QUrl &uri, const DlnaProfile &profile, const QString &title)
{
    const QString name = title.isEmpty() ? QStringLiteral("Cast") : title;
    return QStringLiteral(
               "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
               "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
               "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
               "<item id=\"0\" parentID=\"-1\" restricted=\"1\">"
               "<dc:title>%1</dc:title>"
               "<upnp:class>object.item.videoItem</upnp:class>"
               "<res protocolInfo=\"%2\">%3</res>"
               "</item></DIDL-Lite>")
        .arg(xmlEscape(name), xmlEscape(profile.protocolInfo), xmlEscape(uri.toString()));
}

QByteArray buildSoapEnvelope(const QString &serviceType, const QString &action,
                             const QString &innerXml)
{
    const QString body = QStringLiteral(
                             "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                             "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                             "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                             "<s:Body>"
                             "<u:%1 xmlns:u=\"%2\">%3</u:%1>"
                             "</s:Body></s:Envelope>")
                             .arg(action, serviceType, innerXml);
    return body.toUtf8();
}

QHostAddress pickLocalIpv4(const QHostAddress &peer)
{
    QHostAddress fallback;
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)
            || (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (!isUsableIpv4(ip))
                continue;
            if (!peer.isNull() && peer.isInSubnet(ip, entry.prefixLength())) {
                if (!looksLikeP2pInterface(iface))
                    return ip;
                if (fallback.isNull())
                    fallback = ip;
            }
            if (fallback.isNull() && !looksLikeP2pInterface(iface))
                fallback = ip;
        }
    }
    if (!fallback.isNull())
        return fallback;
    return {};
}

WfdVideoMode dlnaVideoMode(const DisplaySource &source)
{
    WfdVideoMode mode = defaultWfdVideoMode();
    if (!source.isValid())
        return mode;

    int width = source.width;
    int height = source.height;
    if (width > 1280 || height > 720) {
        const double scale = qMin(1280.0 / width, 720.0 / height);
        width = int(width * scale);
        height = int(height * scale);
    }
    mode.width = qMax(2, even(width));
    mode.height = qMax(2, even(height));
    mode.fps = 30;
    return mode;
}

WfdAudioMode dlnaAudioMode(bool enabled)
{
    WfdAudioMode mode;
    if (!enabled)
        return mode;
    mode.codec = WfdAudioMode::Codec::Aac;
    mode.rate = 48000;
    mode.channels = 2;
    return mode;
}