#include "session/micesession.h"

#include "discovery/micediscovery.h"
#include "session/dlnaprofile.h"
#include "session/miceprotocol.h"

#include <QDebug>
#include <QHostAddress>
#include <QSysInfo>

namespace {

bool isIpv4Literal(const QString &value)
{
    QHostAddress address(value);
    return address.protocol() == QAbstractSocket::IPv4Protocol;
}

} // namespace

MiceSession::MiceSession(QObject *parent)
    : QObject(parent)
{
    m_connectTimer.setSingleShot(true);
    connect(&m_connectTimer, &QTimer::timeout, this, &MiceSession::onConnectTimeout);
    connect(&m_socket, &QTcpSocket::connected, this, &MiceSession::onConnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &MiceSession::onReadyRead);
    connect(&m_socket, &QTcpSocket::disconnected, this, [this]() {
        if (m_stopping || m_reported || !m_active)
            return;
        qWarning() << "MICE TCP 7250 closed by sink";
        m_active = false;
        m_reported = true;
        Q_EMIT failed(tr("The display closed the LAN Miracast (MS-MICE) channel."));
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &MiceSession::onError);
}

MiceSession::~MiceSession()
{
    stop();
}

bool MiceSession::active() const
{
    return m_active;
}

QString MiceSession::localIpv4() const
{
    return m_localIpv4;
}

QString MiceSession::remoteIpv4() const
{
    return m_host;
}

void MiceSession::start(const SinkDevice &sink)
{
    stop();
    m_stopping = false;
    m_reported = false;
    m_sink = sink;
    m_sourceId = miceDefaultSourceId();
    m_friendlyName = QSysInfo::machineHostName();
    if (m_friendlyName.isEmpty())
        m_friendlyName = QStringLiteral("Cast");

    QString host = sink.miceHost;
    if (host.isEmpty()) {
        const QString mac = sink.p2pMac.isEmpty() ? sink.address : sink.p2pMac;
        host = MiceDiscovery::ipv4ForHardwareAddress(mac);
    }
    if (isIpv4Literal(host)) {
        connectHost(host);
        return;
    }

    m_lookupNames.clear();
    if (!sink.name.isEmpty() && !sink.name.contains(QLatin1Char(':'))) {
        m_lookupNames.append(sink.name);
        if (!sink.name.endsWith(QLatin1String(".local"), Qt::CaseInsensitive))
            m_lookupNames.append(sink.name + QStringLiteral(".local"));
    }
    m_lookupIndex = 0;
    if (m_lookupNames.isEmpty()) {
        emitUnavailable(tr("No LAN address for this Miracast display."));
        return;
    }
    Q_EMIT statusChanged(tr("Resolving %1 on the LAN…").arg(m_lookupNames.constFirst()));
    resolveNext();
}

void MiceSession::resolveNext()
{
    if (m_lookupIndex >= m_lookupNames.size()) {
        emitUnavailable(tr("Could not resolve a LAN address for %1.").arg(m_sink.name));
        return;
    }
    const QString name = m_lookupNames.at(m_lookupIndex++);
    qInfo() << "MICE lookupHost" << name;
    m_lookupId = QHostInfo::lookupHost(name, this, SLOT(onLookedUp(QHostInfo)));
}

void MiceSession::onLookedUp(const QHostInfo &info)
{
    if (m_stopping || m_reported || m_active)
        return;
    if (info.error() == QHostInfo::NoError) {
        for (const QHostAddress &address : info.addresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
                connectHost(address.toString());
                return;
            }
        }
    }
    qInfo() << "MICE lookup failed" << info.hostName() << info.errorString();
    resolveNext();
}

void MiceSession::connectHost(const QString &host)
{
    m_host = host;
    Q_EMIT statusChanged(tr("Opening Windows Connect on %1:%2…").arg(host).arg(kMicePort));
    qInfo() << "MICE connect" << host << kMicePort;
    m_connectTimer.start(4000);
    m_socket.connectToHost(host, kMicePort);
}

void MiceSession::onConnected()
{
    m_connectTimer.stop();
    const QHostAddress local = m_socket.localAddress();
    m_localIpv4 = local.toIPv4Address() ? local.toString()
                                        : pickLocalIpv4(QHostAddress(m_host)).toString();
    if (m_localIpv4.isEmpty()) {
        emitUnavailable(tr("No LAN IPv4 address the display can reach."));
        stop();
        return;
    }
    m_active = true;
    qInfo() << "MICE TCP 7250 up, local" << m_localIpv4 << "remote" << m_host;
    Q_EMIT statusChanged(tr("LAN Miracast channel open. Waiting for WFD…"));
    Q_EMIT activated(m_localIpv4);
}

void MiceSession::onConnectTimeout()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState)
        return;
    m_socket.abort();
    emitUnavailable(tr("No listener on TCP %1 (MS-MICE).").arg(kMicePort));
}

void MiceSession::onError(QAbstractSocket::SocketError error)
{
    if (m_stopping || m_reported)
        return;
    if (m_active && error == QAbstractSocket::RemoteHostClosedError)
        return;
    m_connectTimer.stop();
    const QString message = m_socket.errorString();
    qWarning() << "MICE socket error" << error << message;
    if (!m_active)
        emitUnavailable(message);
    else {
        m_reported = true;
        Q_EMIT failed(message);
    }
}

bool MiceSession::announce()
{
    if (!m_active || m_socket.state() != QAbstractSocket::ConnectedState)
        return false;
    const QByteArray msg = encodeMiceSourceReady(m_friendlyName, m_sourceId, kWfdRtspPort);
    const qint64 written = m_socket.write(msg);
    m_socket.flush();
    m_announced = written == msg.size();
    qInfo() << "MICE SOURCE_READY" << written << "bytes name" << m_friendlyName;
    if (!m_announced) {
        emitUnavailable(tr("Could not send SOURCE_READY to the display."));
        return false;
    }
    Q_EMIT statusChanged(tr("Told the display we are ready on RTSP %1…").arg(kWfdRtspPort));
    return true;
}

void MiceSession::onReadyRead()
{
    m_rx += m_socket.readAll();
    while (m_rx.size() >= 4) {
        const quint16 size = (quint16(quint8(m_rx.at(0))) << 8) | quint8(m_rx.at(1));
        if (size < 4 || m_rx.size() < size)
            return;
        const quint8 version = quint8(m_rx.at(2));
        const quint8 command = quint8(m_rx.at(3));
        qInfo() << "MICE inbound" << "ver" << version << "cmd" << command << "size" << size;
        m_rx.remove(0, size);
        if (command == kMiceCmdSourceReady || command == kMiceCmdStopProjection)
            continue;
        Q_EMIT failed(tr("The display asked for a Miracast option this sender does not "
                         "implement yet (MS-MICE command %1).")
                          .arg(command));
        return;
    }
}

void MiceSession::sendStop()
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
        return;
    const QByteArray msg = encodeMiceStopProjection(m_friendlyName, m_sourceId);
    m_socket.write(msg);
    m_socket.flush();
    qInfo() << "MICE STOP_PROJECTION sent";
}

void MiceSession::stop()
{
    m_stopping = true;
    m_connectTimer.stop();
    if (m_lookupId) {
        QHostInfo::abortHostLookup(m_lookupId);
        m_lookupId = 0;
    }
    m_lookupNames.clear();
    m_lookupIndex = 0;
    if (m_announced)
        sendStop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.disconnectFromHost();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();
    m_rx.clear();
    m_active = false;
    m_announced = false;
    m_localIpv4.clear();
    m_host.clear();
    m_stopping = false;
}

void MiceSession::emitUnavailable(const QString &message)
{
    if (m_reported)
        return;
    m_reported = true;
    qInfo() << "MICE unavailable" << message;
    Q_EMIT unavailable(message);
}
