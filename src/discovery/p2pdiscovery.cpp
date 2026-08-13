#include "discovery/p2pdiscovery.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QVariant>

namespace {

constexpr auto kNmService = "org.freedesktop.NetworkManager";
constexpr auto kNmPath = "/org/freedesktop/NetworkManager";
constexpr auto kNmIface = "org.freedesktop.NetworkManager";
constexpr auto kDeviceIface = "org.freedesktop.NetworkManager.Device";
constexpr auto kWifiP2PIface = "org.freedesktop.NetworkManager.Device.WifiP2P";
constexpr auto kPeerIface = "org.freedesktop.NetworkManager.WifiP2PPeer";
constexpr auto kPropsIface = "org.freedesktop.DBus.Properties";
constexpr auto kWpaService = "fi.w1.wpa_supplicant1";
constexpr auto kWpaPath = "/fi/w1/wpa_supplicant1";
constexpr auto kWpaIface = "fi.w1.wpa_supplicant1";

// NM_DEVICE_TYPE_WIFI_P2P
constexpr quint32 kDeviceTypeWifiP2P = 30;

// Same WFD Device Information IE GNOME Network Displays advertises as a source.
const char kWfdIesHex[] = "00000600901c4400c8";

} // namespace

P2PDiscovery::P2PDiscovery(QObject *parent)
    : QObject(parent)
{
    m_scanTimer.setSingleShot(true);
    connect(&m_scanTimer, &QTimer::timeout, this, &P2PDiscovery::onScanTimeout);
}

P2PDiscovery::~P2PDiscovery()
{
    stopScan();
}

bool P2PDiscovery::scanning() const
{
    return m_scanning;
}

QVector<SinkDevice> P2PDiscovery::peers() const
{
    QVector<SinkDevice> result;
    result.reserve(m_peers.size());
    for (const auto &sink : m_peers)
        result.append(sink);
    return result;
}

void P2PDiscovery::startScan(int timeoutSeconds)
{
    if (m_scanning)
        stopScan();

    if (!QDBusConnection::systemBus().isConnected()) {
        Q_EMIT errorOccurred(QStringLiteral("System D-Bus is not available."));
        Q_EMIT scanFinished();
        return;
    }

    if (!wifiEnabled()) {
        Q_EMIT errorOccurred(QStringLiteral("Wi-Fi is off. Turn it on to search for Miracast displays."));
        Q_EMIT scanFinished();
        return;
    }

    m_devicePaths = findP2PDevicePaths();
    if (m_devicePaths.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral(
            "No Wi-Fi P2P adapter found. Enable Wi-Fi and use NetworkManager "
            "with wpa_supplicant P2P (not iwd)."));
        Q_EMIT scanFinished();
        return;
    }

    tryAdvertiseWfdIes();

    m_peers.clear();
    Q_EMIT peersChanged();

    const int seconds = qBound(1, timeoutSeconds, 600);
    bool anyStarted = false;
    QString lastError;

    for (const QString &path : m_devicePaths) {
        connectDeviceSignals(path);
        loadExistingPeers(path);

        QDBusInterface iface(QString::fromLatin1(kNmService), path,
                             QString::fromLatin1(kWifiP2PIface),
                             QDBusConnection::systemBus());
        QVariantMap options;
        options.insert(QStringLiteral("timeout"), QVariant::fromValue(qint32(seconds)));
        QDBusReply<void> reply = iface.call(QStringLiteral("StartFind"), QVariant::fromValue(options));
        if (!reply.isValid()) {
            lastError = reply.error().message();
            qWarning() << "StartFind failed on" << path << lastError;
            continue;
        }
        anyStarted = true;
        qInfo() << "P2P StartFind on" << path << "timeout" << seconds;
    }

    if (!anyStarted) {
        disconnectDeviceSignals();
        Q_EMIT errorOccurred(lastError.isEmpty()
                                 ? QStringLiteral("Could not start Wi-Fi Direct find.")
                                 : lastError);
        Q_EMIT scanFinished();
        return;
    }

    m_scanning = true;
    m_scanTimer.start(seconds * 1000);
    Q_EMIT statusChanged(QStringLiteral("Scanning for Miracast displays…"));
}

void P2PDiscovery::stopScan()
{
    m_scanTimer.stop();
    if (!m_scanning && m_devicePaths.isEmpty())
        return;

    stopFindOnDevices();
    disconnectDeviceSignals();
    m_devicePaths.clear();
    if (m_scanning) {
        m_scanning = false;
        Q_EMIT scanFinished();
    }
}

void P2PDiscovery::onPeerAdded(const QDBusObjectPath &path)
{
    upsertPeer(path.path());
}

void P2PDiscovery::onPeerRemoved(const QDBusObjectPath &path)
{
    removePeer(path.path());
}

void P2PDiscovery::onScanTimeout()
{
    stopFindOnDevices();
    disconnectDeviceSignals();
    m_devicePaths.clear();
    m_scanning = false;
    Q_EMIT scanFinished();
}

QStringList P2PDiscovery::findP2PDevicePaths() const
{
    QStringList result;
    QDBusInterface nm(QString::fromLatin1(kNmService),
                      QString::fromLatin1(kNmPath),
                      QString::fromLatin1(kNmIface),
                      QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> reply = nm.call(QStringLiteral("GetAllDevices"));
    if (!reply.isValid()) {
        qWarning() << "GetAllDevices failed" << reply.error().message();
        return result;
    }

    for (const QDBusObjectPath &device : reply.value()) {
        const QVariant typeVar = readProperty(QString::fromLatin1(kNmService),
                                              device.path(),
                                              QString::fromLatin1(kDeviceIface),
                                              QStringLiteral("DeviceType"));
        bool ok = false;
        const quint32 type = typeVar.toUInt(&ok);
        if (ok && type == kDeviceTypeWifiP2P)
            result.append(device.path());
    }
    return result;
}

bool P2PDiscovery::wifiEnabled() const
{
    const QVariant enabled = readProperty(QString::fromLatin1(kNmService),
                                          QString::fromLatin1(kNmPath),
                                          QString::fromLatin1(kNmIface),
                                          QStringLiteral("WirelessEnabled"));
    if (!enabled.isValid())
        return true;
    return enabled.toBool();
}

QVariant P2PDiscovery::readProperty(const QString &service,
                                    const QString &path,
                                    const QString &interface,
                                    const QString &name) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path,
                                                      QString::fromLatin1(kPropsIface),
                                                      QStringLiteral("Get"));
    msg << interface << name;
    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Get" << interface << name << "on" << path << reply.errorMessage();
        return {};
    }
    if (reply.arguments().isEmpty())
        return {};

    const QVariant arg = reply.arguments().constFirst();
    if (arg.canConvert<QDBusVariant>())
        return qvariant_cast<QDBusVariant>(arg).variant();
    return arg;
}

SinkDevice P2PDiscovery::readPeer(const QString &path) const
{
    SinkDevice sink;
    sink.id = path;
    sink.name = readProperty(QString::fromLatin1(kNmService), path,
                             QString::fromLatin1(kPeerIface),
                             QStringLiteral("Name")).toString();
    sink.address = readProperty(QString::fromLatin1(kNmService), path,
                                QString::fromLatin1(kPeerIface),
                                QStringLiteral("HwAddress")).toString();
    const QByteArray ies = readProperty(QString::fromLatin1(kNmService), path,
                                        QString::fromLatin1(kPeerIface),
                                        QStringLiteral("WfdIEs")).toByteArray();
    sink.wfdCapable = !ies.isEmpty();
    if (sink.name.isEmpty())
        sink.name = sink.address.isEmpty() ? path : sink.address;
    return sink;
}

void P2PDiscovery::upsertPeer(const QString &path)
{
    if (path.isEmpty() || path == QLatin1String("/"))
        return;

    const SinkDevice sink = readPeer(path);
    m_peers.insert(path, sink);
    qInfo() << "P2P peer" << sink.name << sink.address << "wfd" << sink.wfdCapable;
    Q_EMIT peersChanged();
}

void P2PDiscovery::removePeer(const QString &path)
{
    if (!m_peers.remove(path))
        return;
    Q_EMIT peersChanged();
}

void P2PDiscovery::connectDeviceSignals(const QString &devicePath)
{
    auto bus = QDBusConnection::systemBus();
    bus.connect(QString::fromLatin1(kNmService), devicePath,
                QString::fromLatin1(kWifiP2PIface), QStringLiteral("PeerAdded"),
                this, SLOT(onPeerAdded(QDBusObjectPath)));
    bus.connect(QString::fromLatin1(kNmService), devicePath,
                QString::fromLatin1(kWifiP2PIface), QStringLiteral("PeerRemoved"),
                this, SLOT(onPeerRemoved(QDBusObjectPath)));
}

void P2PDiscovery::disconnectDeviceSignals()
{
    auto bus = QDBusConnection::systemBus();
    for (const QString &path : m_devicePaths) {
        bus.disconnect(QString::fromLatin1(kNmService), path,
                       QString::fromLatin1(kWifiP2PIface), QStringLiteral("PeerAdded"),
                       this, SLOT(onPeerAdded(QDBusObjectPath)));
        bus.disconnect(QString::fromLatin1(kNmService), path,
                       QString::fromLatin1(kWifiP2PIface), QStringLiteral("PeerRemoved"),
                       this, SLOT(onPeerRemoved(QDBusObjectPath)));
    }
}

void P2PDiscovery::stopFindOnDevices()
{
    for (const QString &path : m_devicePaths) {
        QDBusInterface iface(QString::fromLatin1(kNmService), path,
                             QString::fromLatin1(kWifiP2PIface),
                             QDBusConnection::systemBus());
        iface.call(QStringLiteral("StopFind"));
    }
}

void P2PDiscovery::tryAdvertiseWfdIes()
{
    const QByteArray ies = QByteArray::fromHex(kWfdIesHex);
    QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(kWpaService),
                                                      QString::fromLatin1(kWpaPath),
                                                      QString::fromLatin1(kPropsIface),
                                                      QStringLiteral("Set"));
    msg << QString::fromLatin1(kWpaIface)
        << QStringLiteral("WFDIEs")
        << QVariant::fromValue(QDBusVariant(QVariant::fromValue(ies)));
    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage)
        qWarning() << "Could not set wpa_supplicant WFDIEs" << reply.errorMessage();
    else
        qInfo() << "Advertised WFD IEs on wpa_supplicant";
}

void P2PDiscovery::loadExistingPeers(const QString &devicePath)
{
    const QVariant peersVar = readProperty(QString::fromLatin1(kNmService),
                                           devicePath,
                                           QString::fromLatin1(kWifiP2PIface),
                                           QStringLiteral("Peers"));
    const auto paths = qdbus_cast<QList<QDBusObjectPath>>(peersVar);
    for (const QDBusObjectPath &path : paths)
        upsertPeer(path.path());
}
