#include "discovery/dlnadiscovery.h"

#include "discovery/dlnadescription.h"
#include "discovery/dlnassdp.h"
#include "session/dlnaprofile.h"

#include <QDebug>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

constexpr quint16 kSsdpPort = 1900;
const QHostAddress kSsdpGroup(QStringLiteral("239.255.255.250"));

SinkDevice toSink(const DlnaRendererDesc &desc, const QHostAddress &peer)
{
    SinkDevice sink;
    sink.protocol = CastProtocol::Dlna;
    sink.udn = desc.udn;
    sink.id = QStringLiteral("dlna:%1").arg(desc.udn);
    sink.name = desc.name;
    sink.locationUrl = desc.location;
    sink.avTransportUrl = desc.avTransport;
    sink.connectionManagerUrl = desc.connectionManager;
    sink.address = desc.location.host();
    if (sink.address.isEmpty() && !peer.isNull())
        sink.address = peer.toString();
    return sink;
}

} // namespace

DlnaDiscovery::DlnaDiscovery(QObject *parent)
    : QObject(parent)
{
    m_scanTimer.setSingleShot(true);
    connect(&m_scanTimer, &QTimer::timeout, this, &DlnaDiscovery::onScanTimeout);
    m_repeatTimer.setSingleShot(true);
    connect(&m_repeatTimer, &QTimer::timeout, this, &DlnaDiscovery::sendMsearch);
    connect(&m_socket, &QUdpSocket::readyRead, this, &DlnaDiscovery::onReadyRead);
}

DlnaDiscovery::~DlnaDiscovery()
{
    stopScan();
}

bool DlnaDiscovery::scanning() const
{
    return m_scanning;
}

QVector<SinkDevice> DlnaDiscovery::renderers() const
{
    QVector<SinkDevice> result;
    result.reserve(m_renderers.size());
    for (const auto &sink : m_renderers)
        result.append(sink);
    return result;
}

void DlnaDiscovery::startScan(int timeoutSeconds)
{
    if (m_scanning)
        stopScan();

    m_renderers.clear();
    m_pendingLocations.clear();
    Q_EMIT renderersChanged();

    if (m_socket.state() != QAbstractSocket::BoundState) {
        if (!m_socket.bind(QHostAddress::AnyIPv4, 0,
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            Q_EMIT errorOccurred(tr("Could not listen for DLNA / SSDP replies (%1).")
                                     .arg(m_socket.errorString()));
            Q_EMIT scanFinished();
            return;
        }
        m_socket.setSocketOption(QAbstractSocket::MulticastTtlOption, 4);
    }

    m_scanning = true;
    const int seconds = qBound(3, timeoutSeconds, 60);
    m_scanTimer.start(seconds * 1000);
    sendMsearch();
    m_repeatTimer.start(3000);
    Q_EMIT statusChanged(tr("Scanning for DLNA renderers…"));
}

void DlnaDiscovery::stopScan()
{
    m_scanTimer.stop();
    m_repeatTimer.stop();
    m_pendingLocations.clear();
    if (!m_scanning)
        return;
    m_scanning = false;
    Q_EMIT scanFinished();
}

void DlnaDiscovery::onScanTimeout()
{
    m_repeatTimer.stop();
    if (!m_scanning)
        return;
    m_scanning = false;
    Q_EMIT scanFinished();
}

void DlnaDiscovery::sendMsearch()
{
    const QByteArray payload = buildSsdpMsearch(3);
    int sent = 0;
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)
            || (iface.flags() & QNetworkInterface::IsLoopBack)
            || (iface.flags() & QNetworkInterface::IsPointToPoint))
            continue;
        if (iface.name().startsWith(QLatin1String("p2p"))
            || iface.name().startsWith(QLatin1String("wifi-p2p")))
            continue;
        bool hasV4 = false;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol
                && !entry.ip().isLinkLocal()) {
                hasV4 = true;
                break;
            }
        }
        if (!hasV4)
            continue;
        m_socket.setMulticastInterface(iface);
        if (m_socket.writeDatagram(payload, kSsdpGroup, kSsdpPort) > 0)
            ++sent;
    }
    if (sent == 0)
        m_socket.writeDatagram(payload, kSsdpGroup, kSsdpPort);
    qInfo() << "SSDP M-SEARCH MediaRenderer:1 on" << sent << "interface(s)";
}

void DlnaDiscovery::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram();
        handleSsdp(datagram.data(), datagram.senderAddress());
    }
}

void DlnaDiscovery::handleSsdp(const QByteArray &datagram, const QHostAddress &peer)
{
    const SsdpResponse parsed = parseSsdpDatagram(datagram);
    if (!parsed.location.isValid())
        return;
    const QString st = parsed.st;
    if (!st.isEmpty()
        && !st.contains(QLatin1String("MediaRenderer"), Qt::CaseInsensitive)
        && st.compare(QLatin1String("upnp:rootdevice"), Qt::CaseInsensitive) != 0
        && !st.startsWith(QLatin1String("uuid:"), Qt::CaseInsensitive)) {
        return;
    }
    fetchDescription(parsed.location, peer);
}

void DlnaDiscovery::fetchDescription(const QUrl &location, const QHostAddress &peer)
{
    const QString key = location.toString();
    if (m_pendingLocations.contains(key))
        return;
    for (const auto &existing : m_renderers) {
        if (existing.locationUrl == location)
            return;
    }

    m_pendingLocations.insert(key);
    QNetworkRequest request(location);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ot-cast/0.1"));
    request.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, location, peer]() {
        reply->deleteLater();
        m_pendingLocations.remove(location.toString());
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "DLNA description" << location << reply->errorString();
            return;
        }
        const auto found = parseMediaRenderers(reply->readAll(), location);
        if (found.isEmpty())
            return;
        for (const DlnaRendererDesc &desc : found) {
            const SinkDevice sink = toSink(desc, peer);
            ingestRenderer(sink);
            queryProtocolInfo(sink.id);
        }
    });
}

void DlnaDiscovery::ingestRenderer(const SinkDevice &sink)
{
    if (sink.id.isEmpty())
        return;
    m_renderers.insert(sink.id, sink);
    const bool classified =
        !sink.protocolInfo.isEmpty() || !sink.connectionManagerUrl.isValid();
    if (classified) {
        qInfo() << "device-matrix"
                << "protocol=dlna"
                << "name=" + sink.name
                << "address=" + sink.address
                << "hint=" + dlnaMediaKindKey(sink.dlnaMedia)
                << "summary=" + sink.dlnaMediaSummary
                << "result=untested";
    }
    qInfo() << "DLNA renderer" << sink.name << sink.address << sink.avTransportUrl
            << dlnaMediaKindKey(sink.dlnaMedia) << sink.dlnaMediaSummary;
    Q_EMIT renderersChanged();
}

void DlnaDiscovery::queryProtocolInfo(const QString &sinkId)
{
    const SinkDevice sink = m_renderers.value(sinkId);
    if (!sink.connectionManagerUrl.isValid())
        return;

    QNetworkRequest request(sink.connectionManagerUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("text/xml; charset=\"utf-8\""));
    request.setRawHeader(
        "SOAPAction",
        QByteArray("\"urn:schemas-upnp-org:service:ConnectionManager:1#GetProtocolInfo\""));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ot-cast/0.1"));
    request.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam.post(
        request,
        buildSoapEnvelope(QStringLiteral("urn:schemas-upnp-org:service:ConnectionManager:1"),
                          QStringLiteral("GetProtocolInfo"), QString()));
    connect(reply, &QNetworkReply::finished, this, [this, reply, sinkId]() {
        reply->deleteLater();
        if (!m_renderers.contains(sinkId))
            return;
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "GetProtocolInfo" << sinkId << reply->errorString();
            return;
        }
        const QString sinkInfo = parseConnectionManagerSink(reply->readAll());
        if (sinkInfo.isEmpty())
            return;
        SinkDevice updated = m_renderers.value(sinkId);
        applyDlnaProtocolInfo(&updated, sinkInfo);
        ingestRenderer(updated);
    });
}