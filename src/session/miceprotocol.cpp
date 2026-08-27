#include "session/miceprotocol.h"

#include <QHash>
#include <QStringList>

#include <utility>

namespace {

void appendBe16(QByteArray &out, quint16 value)
{
    out.append(char(value >> 8));
    out.append(char(value & 0xff));
}

void appendTlv(QByteArray &out, quint8 type, const QByteArray &value)
{
    out.append(char(type));
    appendBe16(out, quint16(value.size()));
    out.append(value);
}

QByteArray encodeMiceMessage(quint8 command, const QString &friendlyName,
                             const QByteArray &sourceId, bool withRtspPort, quint16 rtspPort)
{
    QByteArray id = sourceId;
    if (id.size() < kMiceSourceIdSize)
        id = id + QByteArray(kMiceSourceIdSize - id.size(), '\0');
    else if (id.size() > kMiceSourceIdSize)
        id = id.left(kMiceSourceIdSize);

    QByteArray name = encodeUtf16Le(friendlyName);
    if (name.size() > kMiceFriendlyNameMaxBytes)
        name = name.left(kMiceFriendlyNameMaxBytes & ~1);

    QByteArray tlvs;
    appendTlv(tlvs, 0x00, name);
    if (withRtspPort) {
        QByteArray port;
        appendBe16(port, rtspPort);
        appendTlv(tlvs, 0x02, port);
    }
    appendTlv(tlvs, 0x03, id);

    QByteArray msg;
    appendBe16(msg, 0);
    msg.append(char(kMiceVersion));
    msg.append(char(command));
    msg.append(tlvs);
    const quint16 size = quint16(msg.size());
    msg[0] = char(size >> 8);
    msg[1] = char(size & 0xff);
    return msg;
}

void appendDnsLabel(QByteArray &out, const QByteArray &label)
{
    out.append(char(label.size()));
    out.append(label);
}

bool parseDnsName(const QByteArray &pkt, int *offset, QString *out, int depth = 0)
{
    if (!offset || *offset < 0 || depth > 10)
        return false;

    QStringList labels;
    int pos = *offset;
    bool jumped = false;
    int returnPos = *offset;

    while (true) {
        if (pos >= pkt.size())
            return false;
        const quint8 len = quint8(pkt.at(pos));
        if (len == 0) {
            pos += 1;
            if (!jumped)
                returnPos = pos;
            break;
        }
        if ((len & 0xC0) == 0xC0) {
            if (pos + 1 >= pkt.size())
                return false;
            const int ptr = ((len & 0x3F) << 8) | quint8(pkt.at(pos + 1));
            if (!jumped)
                returnPos = pos + 2;
            jumped = true;
            pos = ptr;
            continue;
        }
        if ((len & 0xC0) != 0)
            return false;
        pos += 1;
        if (pos + len > pkt.size())
            return false;
        labels.append(QString::fromLatin1(pkt.mid(pos, len)));
        pos += len;
        if (!jumped)
            returnPos = pos;
    }
    *offset = returnPos;
    if (out)
        *out = labels.join(QLatin1Char('.'));
    return true;
}

QString instanceName(const QString &owner)
{
    const QString suffix = QStringLiteral("._display._tcp.local");
    if (owner.endsWith(suffix, Qt::CaseInsensitive))
        return owner.left(owner.size() - suffix.size());
    const QString suffix2 = QStringLiteral("_display._tcp.local");
    if (owner.endsWith(suffix2, Qt::CaseInsensitive)) {
        QString name = owner.left(owner.size() - suffix2.size());
        while (name.endsWith(QLatin1Char('.')))
            name.chop(1);
        return name;
    }
    return {};
}

} // namespace

QByteArray miceDefaultSourceId()
{
    return QByteArrayLiteral("01tool.cast.src!");
}

QByteArray encodeUtf16Le(const QString &text)
{
    QByteArray out;
    out.reserve(text.size() * 2);
    const ushort *utf16 = text.utf16();
    for (int i = 0; i < text.size(); ++i) {
        const quint16 unit = utf16[i];
        out.append(char(unit & 0xff));
        out.append(char(unit >> 8));
    }
    return out;
}

QByteArray encodeMiceSourceReady(const QString &friendlyName, const QByteArray &sourceId,
                                 quint16 rtspPort)
{
    return encodeMiceMessage(kMiceCmdSourceReady, friendlyName, sourceId, true, rtspPort);
}

QByteArray encodeMiceStopProjection(const QString &friendlyName, const QByteArray &sourceId)
{
    return encodeMiceMessage(kMiceCmdStopProjection, friendlyName, sourceId, false, 0);
}

QString normalizeMac(const QString &mac)
{
    QString hex;
    hex.reserve(12);
    for (QChar c : mac) {
        if (c.isDigit() || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
            || (c >= QLatin1Char('A') && c <= QLatin1Char('F')))
            hex.append(c.toLower());
    }
    return hex;
}

QString toggleMacLaa(const QString &mac)
{
    const QString hex = normalizeMac(mac);
    if (hex.size() != 12)
        return {};
    bool ok = false;
    const int first = hex.left(2).toInt(&ok, 16);
    if (!ok)
        return {};
    const int toggled = first ^ 0x02;
    return QStringLiteral("%1%2")
        .arg(toggled, 2, 16, QLatin1Char('0'))
        .arg(hex.mid(2))
        .toLower();
}

bool macsRelated(const QString &a, const QString &b)
{
    const QString na = normalizeMac(a);
    const QString nb = normalizeMac(b);
    if (na.size() != 12 || nb.size() != 12)
        return false;
    if (na == nb)
        return true;
    return toggleMacLaa(na) == nb;
}

QVector<MiceArpRow> parseArpTable(const QByteArray &text)
{
    QVector<MiceArpRow> rows;
    const QList<QByteArray> lines = text.split('\n');
    for (const QByteArray &raw : lines) {
        const QByteArray line = raw.trimmed();
        if (line.isEmpty() || line.startsWith("IP address"))
            continue;
        QList<QByteArray> parts;
        for (const QByteArray &part : line.split(' ')) {
            if (!part.isEmpty())
                parts.append(part);
        }
        if (parts.size() < 4)
            continue;
        MiceArpRow row;
        row.ip = QString::fromLatin1(parts.at(0));
        bool ok = false;
        row.flags = QString::fromLatin1(parts.at(2)).toUInt(&ok, 0);
        row.mac = QString::fromLatin1(parts.at(3)).toLower();
        if (row.ip.isEmpty() || normalizeMac(row.mac).size() != 12)
            continue;
        if (normalizeMac(row.mac) == QLatin1String("000000000000"))
            continue;
        rows.append(row);
    }
    return rows;
}

QString ipv4ForHardwareAddress(const QString &mac, const QByteArray &arpText)
{
    if (normalizeMac(mac).size() != 12)
        return {};
    const auto rows = parseArpTable(arpText);
    QString incomplete;
    for (const MiceArpRow &row : rows) {
        if (!macsRelated(row.mac, mac))
            continue;
        if (row.flags & 0x02)
            return row.ip;
        if (incomplete.isEmpty())
            incomplete = row.ip;
    }
    return incomplete;
}

QByteArray buildMdnsDisplayPtrQuery()
{
    QByteArray pkt;
    appendBe16(pkt, 0);
    appendBe16(pkt, 0);
    appendBe16(pkt, 1);
    appendBe16(pkt, 0);
    appendBe16(pkt, 0);
    appendBe16(pkt, 0);
    appendDnsLabel(pkt, QByteArrayLiteral("_display"));
    appendDnsLabel(pkt, QByteArrayLiteral("_tcp"));
    appendDnsLabel(pkt, QByteArrayLiteral("local"));
    pkt.append('\0');
    appendBe16(pkt, 12);
    appendBe16(pkt, 0x8001);
    return pkt;
}

QVector<MiceDnsService> parseMdnsDisplayServices(const QByteArray &packet, const QString &senderIpv4)
{
    if (packet.size() < 12)
        return {};

    const auto u16 = [&packet](int off) -> quint16 {
        return (quint16(quint8(packet.at(off))) << 8) | quint8(packet.at(off + 1));
    };

    const int qd = u16(4);
    const int an = u16(6);
    const int ns = u16(8);
    const int ar = u16(10);
    int offset = 12;

    for (int i = 0; i < qd; ++i) {
        QString name;
        if (!parseDnsName(packet, &offset, &name))
            return {};
        offset += 4;
        if (offset > packet.size())
            return {};
    }

    QHash<QString, MiceDnsService> byInstance;
    QHash<QString, QString> aRecords;

    const int recordCount = an + ns + ar;
    for (int i = 0; i < recordCount; ++i) {
        QString owner;
        if (!parseDnsName(packet, &offset, &owner))
            break;
        if (offset + 10 > packet.size())
            break;
        const quint16 type = u16(offset);
        offset += 2;
        offset += 2;
        offset += 4;
        const quint16 rdlen = u16(offset);
        offset += 2;
        if (offset + rdlen > packet.size())
            break;
        const int rdata = offset;
        const int next = offset + rdlen;

        if (type == 1 && rdlen == 4) {
            const QString ip = QStringLiteral("%1.%2.%3.%4")
                                   .arg(quint8(packet.at(rdata)))
                                   .arg(quint8(packet.at(rdata + 1)))
                                   .arg(quint8(packet.at(rdata + 2)))
                                   .arg(quint8(packet.at(rdata + 3)));
            aRecords.insert(owner.toLower(), ip);
        } else if (type == 12) {
            int nameOff = rdata;
            QString target;
            if (parseDnsName(packet, &nameOff, &target)) {
                QString name = instanceName(target);
                if (name.isEmpty())
                    name = instanceName(owner);
                if (!name.isEmpty()) {
                    MiceDnsService &svc = byInstance[name.toLower()];
                    svc.name = name;
                }
            }
        } else if (type == 33 && rdlen >= 6) {
            const QString name = instanceName(owner);
            int targetOff = rdata + 6;
            QString host;
            parseDnsName(packet, &targetOff, &host);
            if (!name.isEmpty()) {
                MiceDnsService &svc = byInstance[name.toLower()];
                svc.name = name;
                svc.port = u16(rdata + 4);
                svc.host = host;
            }
        } else if (type == 16) {
            const QString name = instanceName(owner);
            int pos = rdata;
            QString p2pMac;
            while (pos < next) {
                const quint8 len = quint8(packet.at(pos++));
                if (pos + len > next)
                    break;
                const QByteArray kv = packet.mid(pos, len);
                pos += len;
                const int eq = kv.indexOf('=');
                if (eq <= 0)
                    continue;
                const QByteArray key = kv.left(eq);
                if (key.compare("p2pmac", Qt::CaseInsensitive) == 0)
                    p2pMac = QString::fromUtf8(kv.mid(eq + 1));
            }
            if (!name.isEmpty()) {
                MiceDnsService &svc = byInstance[name.toLower()];
                svc.name = name;
                if (!p2pMac.isEmpty())
                    svc.p2pMac = p2pMac;
            }
        }
        offset = next;
    }

    QVector<MiceDnsService> result;
    for (MiceDnsService svc : std::as_const(byInstance)) {
        if (svc.name.isEmpty())
            continue;
        if (svc.ipv4.isEmpty() && !svc.host.isEmpty())
            svc.ipv4 = aRecords.value(svc.host.toLower());
        if (svc.ipv4.isEmpty())
            svc.ipv4 = senderIpv4;
        if (svc.port == 0)
            svc.port = kMicePort;
        result.append(svc);
    }
    return result;
}
