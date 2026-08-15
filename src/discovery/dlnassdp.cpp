#include "discovery/dlnassdp.h"

#include <QByteArrayList>
#include <QHash>

QByteArray buildSsdpMsearch(int mxSeconds)
{
    const int mx = qBound(1, mxSeconds, 5);
    return QByteArray("M-SEARCH * HTTP/1.1\r\n"
                      "HOST: 239.255.255.250:1900\r\n"
                      "MAN: \"ssdp:discover\"\r\n"
                      "MX: ")
        + QByteArray::number(mx)
        + QByteArray("\r\n"
                     "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
                     "\r\n");
}

SsdpResponse parseSsdpDatagram(const QByteArray &datagram)
{
    SsdpResponse out;
    const QByteArray text = datagram;
    const int headerEnd = text.indexOf("\r\n");
    if (headerEnd < 0)
        return {};
    const QByteArray start = text.left(headerEnd).toUpper();
    const bool okReply = start.startsWith("HTTP/1.");
    const bool notify = start.startsWith("NOTIFY");
    if (!okReply && !notify)
        return {};

    QHash<QByteArray, QByteArray> headers;
    const QByteArrayList lines = text.split('\n');
    for (int i = 1; i < lines.size(); ++i) {
        QByteArray line = lines.at(i).trimmed();
        if (line.isEmpty())
            break;
        const int colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        const QByteArray key = line.left(colon).trimmed().toLower();
        const QByteArray value = line.mid(colon + 1).trimmed();
        headers.insert(key, value);
    }

    const QByteArray loc = headers.value("location");
    if (loc.isEmpty())
        return {};
    out.location = QUrl(QString::fromUtf8(loc));
    if (!out.location.isValid() || out.location.host().isEmpty())
        return {};
    out.usn = QString::fromUtf8(headers.value("usn"));
    out.st = QString::fromUtf8(headers.value("st"));
    if (out.st.isEmpty())
        out.st = QString::fromUtf8(headers.value("nt"));
    out.server = QString::fromUtf8(headers.value("server"));
    return out;
}