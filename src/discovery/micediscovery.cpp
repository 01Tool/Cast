#include "discovery/micediscovery.h"

#include "session/miceprotocol.h"

#include <QDebug>
#include <QFile>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QVariant>

namespace {

const QHostAddress kMdnsGroup(QStringLiteral("224.0.0.251"));
constexpr quint16 kMdnsPort = 5353;

bool isStaInterface(const QNetworkInterface &iface)
{
    if (!(iface.flags() & QNetworkInterface::IsUp)
        || (iface.flags() & QNetworkInterface::IsLoopBack)
        || (iface.flags() & QNetworkInterface::IsPointToPoint))
        return false;
    if (iface.name().startsWith(QLatin1String("p2p"))
        || iface.name().startsWith(QLatin1String("wifi-p2p")))
        return false;
    for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLinkLocal())
            return true;
    }
    return false;
}

SinkDevice toSink(const MiceDnsService &svc)
{
    SinkDevice sink;
    sink.protocol = CastProtocol::Miracast;
    sink.wfdCapable = true;
    sink.miceCapable = true;
    sink.name = svc.name;
    sink.miceHost = svc.ipv4;
    sink.address = svc.ipv4;
    sink.p2pMac = svc.p2pMac;
    sink.id = QStringLiteral("mice:%1").arg(svc.ipv4.isEmpty() ? svc.name : svc.ipv4);
    if (sink.name.isEmpty())
        sink.name = sink.address;
    return sink;
}

} // namespace

MiceDiscovery::MiceDiscovery(QObject *parent)
    : QObject(parent)
{
    m_scanTimer.setSingleShot(true);
    connect(&m_scanTimer, &QTimer::timeout, this, &MiceDiscovery::onScanTimeout);
    m_repeatTimer.setSingleShot(true);
    connect(&m_repeatTimer, &QTimer::timeout, this, &MiceDiscovery::sendQuery);
    connect(&m_socket, &QUdpSocket::readyRead, this, &MiceDiscovery::onReadyRead);
}

MiceDiscovery::~MiceDiscovery()
{
    stopScan();
}

bool MiceDiscovery::scanning() const
{
    return m_scanning;
}

QVector<SinkDevice> MiceDiscovery::sinks() const
{
    QVector<SinkDevice> result;
    result.reserve(m_sinks.size());
    for (const auto &sink : m_sinks)
        result.append(sink);
    return result;
}

QString MiceDiscovery::ipv4ForHardwareAddress(const QString &mac)
{
    QFile file(QStringLiteral("/proc/net/arp"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return ::ipv4ForHardwareAddress(mac, file.readAll());
}

bool MiceDiscovery::matchesP2p(const SinkDevice &mice, const SinkDevice &p2p)
{
    const QString p2pMac = p2p.p2pMac.isEmpty() ? p2p.address : p2p.p2pMac;
    if (!mice.p2pMac.isEmpty() && macsRelated(mice.p2pMac, p2pMac))
        return true;
    if (!mice.name.isEmpty() && !p2p.name.isEmpty()
        && mice.name.compare(p2p.name, Qt::CaseInsensitive) == 0)
        return true;
    return false;
}

void MiceDiscovery::startScan(int timeoutSeconds)
{
    if (m_scanning)
        stopScan();

    m_sinks.clear();
    Q_EMIT sinksChanged();

    if (!bindSocket()) {
        qWarning() << "MICE mDNS bind failed" << m_socket.errorString();
        Q_EMIT scanFinished();
        return;
    }

    m_scanning = true;
    const int seconds = qBound(2, timeoutSeconds, 60);
    m_scanTimer.start(seconds * 1000);
    sendQuery();
    m_repeatTimer.start(2000);
    Q_EMIT statusChanged(tr("Scanning for LAN Miracast (Windows Connect)…"));
}

void MiceDiscovery::stopScan()
{
    m_scanTimer.stop();
    m_repeatTimer.stop();
    if (!m_scanning)
        return;
    m_scanning = false;
    Q_EMIT scanFinished();
}

void MiceDiscovery::onScanTimeout()
{
    m_repeatTimer.stop();
    if (!m_scanning)
        return;
    m_scanning = false;
    Q_EMIT scanFinished();
}

bool MiceDiscovery::bindSocket()
{
    if (m_socket.state() == QAbstractSocket::BoundState)
        return true;

    const auto flags = QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint;
    if (!m_socket.bind(QHostAddress::AnyIPv4, kMdnsPort, flags)) {
        if (!m_socket.bind(QHostAddress::AnyIPv4, 0, flags))
            return false;
    }
    m_socket.setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(1));
    m_socket.setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(0));
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!isStaInterface(iface))
            continue;
        if (!m_socket.joinMulticastGroup(kMdnsGroup, iface))
            qDebug() << "MICE mDNS join failed on" << iface.name() << m_socket.errorString();
    }
    return true;
}

void MiceDiscovery::sendQuery()
{
    const QByteArray payload = buildMdnsDisplayPtrQuery();
    int sent = 0;
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!isStaInterface(iface))
            continue;
        m_socket.setMulticastInterface(iface);
        if (m_socket.writeDatagram(payload, kMdnsGroup, kMdnsPort) > 0)
            ++sent;
    }
    if (sent == 0)
        m_socket.writeDatagram(payload, kMdnsGroup, kMdnsPort);
    qInfo() << "mDNS PTR _display._tcp.local on" << sent << "interface(s)";
}

void MiceDiscovery::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram();
        const QString sender = datagram.senderAddress().toString();
        const auto services = parseMdnsDisplayServices(datagram.data(), sender);
        for (const MiceDnsService &svc : services)
            ingest(svc, sender);
    }
}

void MiceDiscovery::ingest(const MiceDnsService &svc, const QString &senderIpv4)
{
    MiceDnsService filled = svc;
    if (filled.ipv4.isEmpty())
        filled.ipv4 = senderIpv4;
    if (filled.name.isEmpty() || filled.ipv4.isEmpty())
        return;

    const SinkDevice sink = toSink(filled);
    const QString key = sink.id;
    const auto it = m_sinks.find(key);
    if (it != m_sinks.end()) {
        if (it->p2pMac.isEmpty() && !sink.p2pMac.isEmpty()) {
            it->p2pMac = sink.p2pMac;
            Q_EMIT sinksChanged();
        }
        return;
    }
    m_sinks.insert(key, sink);
    qInfo() << "MICE mDNS" << sink.name << sink.miceHost << "p2pMAC" << sink.p2pMac;
    Q_EMIT sinksChanged();
}
