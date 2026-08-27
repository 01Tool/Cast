#include "session/p2psession.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDebug>
#include <QProcessEnvironment>

namespace {

constexpr auto kNmService = "org.freedesktop.NetworkManager";
constexpr auto kNmPath = "/org/freedesktop/NetworkManager";
constexpr auto kNmIface = "org.freedesktop.NetworkManager";
constexpr auto kActiveIface = "org.freedesktop.NetworkManager.Connection.Active";
constexpr auto kIp4Iface = "org.freedesktop.NetworkManager.IP4Config";
constexpr auto kPropsIface = "org.freedesktop.DBus.Properties";

// Same source WFD IE GNOME Network Displays puts on the wifi-p2p profile.
const char kWfdIesHex[] = "00000600901c4400c8";

// NM_ACTIVE_CONNECTION_STATE_ACTIVATED
constexpr quint32 kActiveStateActivated = 2;
constexpr quint32 kActiveStateDeactivated = 4;

using ConnectionMap = QMap<QString, QVariantMap>;

} // namespace

P2PSession::P2PSession(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<ConnectionMap>();
    m_statePoll.setInterval(1000);
    connect(&m_statePoll, &QTimer::timeout, this, &P2PSession::pollActiveState);
}

P2PSession::~P2PSession()
{
    deactivate();
}

bool P2PSession::active() const
{
    return m_active;
}

QString P2PSession::localIpv4() const
{
    return m_localIpv4;
}

QString P2PSession::lastError() const
{
    return m_lastError;
}

void P2PSession::activate(const SinkDevice &sink)
{
    if (m_active || !m_activePath.isEmpty())
        deactivate();

    if (sink.address.isEmpty() || sink.id.isEmpty() || sink.p2pDevicePath.isEmpty()) {
        m_lastError = tr("Sink is missing P2P address or device path.");
        Q_EMIT failed(m_lastError);
        return;
    }

    ConnectionMap conn;

    QVariantMap general;
    general.insert(QStringLiteral("type"), QStringLiteral("wifi-p2p"));
    general.insert(QStringLiteral("id"), QStringLiteral("Cast %1").arg(sink.address));
    general.insert(QStringLiteral("autoconnect"), false);
    const QString user = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USER"));
    if (!user.isEmpty()) {
        general.insert(QStringLiteral("permissions"),
                       QStringList{QStringLiteral("user:%1:").arg(user)});
    }
    conn.insert(QStringLiteral("connection"), general);

    QVariantMap p2p;
    p2p.insert(QStringLiteral("peer"), sink.address);
    p2p.insert(QStringLiteral("wfd-ies"), QByteArray::fromHex(kWfdIesHex));
    // NM_SETTING_WIRELESS_SECURITY_WPS_METHOD_AUTO: NM picks PBC or PIN.
    p2p.insert(QStringLiteral("wps-method"), quint32(0x2));
    conn.insert(QStringLiteral("wifi-p2p"), p2p);

    QVariantMap ipv4;
    ipv4.insert(QStringLiteral("method"), QStringLiteral("auto"));
    ipv4.insert(QStringLiteral("never-default"), true);
    conn.insert(QStringLiteral("ipv4"), ipv4);

    QVariantMap ipv6;
    ipv6.insert(QStringLiteral("method"), QStringLiteral("auto"));
    ipv6.insert(QStringLiteral("never-default"), true);
    ipv6.insert(QStringLiteral("may-fail"), true);
    conn.insert(QStringLiteral("ipv6"), ipv6);

    QVariantMap options;
    options.insert(QStringLiteral("persist"), QStringLiteral("volatile"));
    options.insert(QStringLiteral("bind-activation"), QStringLiteral("dbus-client"));

    QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(kNmService),
                                                      QString::fromLatin1(kNmPath),
                                                      QString::fromLatin1(kNmIface),
                                                      QStringLiteral("AddAndActivateConnection2"));
    msg << QVariant::fromValue(conn)
        << QVariant::fromValue(QDBusObjectPath(sink.p2pDevicePath))
        << QVariant::fromValue(QDBusObjectPath(sink.id))
        << QVariant::fromValue(options);

    Q_EMIT statusChanged(tr("Forming Wi-Fi Direct group…"));
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &P2PSession::onAddFinished);
}

void P2PSession::deactivate()
{
    m_statePoll.stop();
    unwatchActiveConnection();
    if (!m_activePath.isEmpty()) {
        QDBusInterface nm(QString::fromLatin1(kNmService),
                          QString::fromLatin1(kNmPath),
                          QString::fromLatin1(kNmIface),
                          QDBusConnection::systemBus());
        nm.call(QStringLiteral("DeactivateConnection"),
                QVariant::fromValue(QDBusObjectPath(m_activePath)));
    }
    const bool wasActive = m_active;
    m_activePath.clear();
    m_localIpv4.clear();
    m_active = false;
    if (wasActive)
        Q_EMIT deactivated();
}

void P2PSession::onAddFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath, QDBusObjectPath, QVariantMap> reply = *watcher;
    if (reply.isError()) {
        m_lastError = reply.error().message();
        qWarning() << "AddAndActivateConnection2" << m_lastError;
        Q_EMIT failed(m_lastError);
        return;
    }

    m_activePath = reply.argumentAt<1>().path();
    qInfo() << "P2P active connection" << m_activePath;
    watchActiveConnection(m_activePath);
    m_statePoll.start();
    pollActiveState();
}

void P2PSession::onActiveProperties(const QString &interface,
                                    const QVariantMap &changed,
                                    const QStringList &invalidated)
{
    Q_UNUSED(invalidated);
    if (interface != QLatin1String(kActiveIface))
        return;
    if (!changed.contains(QStringLiteral("State")))
        return;

    bool ok = false;
    const quint32 state = changed.value(QStringLiteral("State")).toUInt(&ok);
    if (!ok)
        return;
    handleActiveState(state);
}

void P2PSession::onActiveStateChanged(uint state, uint reason)
{
    qInfo() << "P2P Active.StateChanged" << state << "reason" << reason;
    handleActiveState(state);
}

void P2PSession::pollActiveState()
{
    if (m_activePath.isEmpty())
        return;
    const QVariant stateVar = readProperty(m_activePath,
                                           QString::fromLatin1(kActiveIface),
                                           QStringLiteral("State"));
    bool ok = false;
    const quint32 state = stateVar.toUInt(&ok);
    if (!ok)
        return;
    qInfo() << "P2P active state poll" << state << "ipv4" << queryLocalIpv4();
    handleActiveState(state);
}

QVariant P2PSession::readProperty(const QString &path,
                                  const QString &interface,
                                  const QString &name) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(kNmService),
                                                      path,
                                                      QString::fromLatin1(kPropsIface),
                                                      QStringLiteral("Get"));
    msg << interface << name;
    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty())
        return {};
    const QVariant arg = reply.arguments().constFirst();
    if (arg.canConvert<QDBusVariant>())
        return qvariant_cast<QDBusVariant>(arg).variant();
    return arg;
}

void P2PSession::watchActiveConnection(const QString &path)
{
    auto bus = QDBusConnection::systemBus();
    bus.connect(QString::fromLatin1(kNmService), path, QString::fromLatin1(kPropsIface),
                QStringLiteral("PropertiesChanged"), this,
                SLOT(onActiveProperties(QString,QVariantMap,QStringList)));
    bus.connect(QString::fromLatin1(kNmService), path, QString::fromLatin1(kActiveIface),
                QStringLiteral("StateChanged"), this,
                SLOT(onActiveStateChanged(uint,uint)));
}

void P2PSession::unwatchActiveConnection()
{
    if (m_activePath.isEmpty())
        return;
    auto bus = QDBusConnection::systemBus();
    bus.disconnect(QString::fromLatin1(kNmService), m_activePath, QString::fromLatin1(kPropsIface),
                   QStringLiteral("PropertiesChanged"), this,
                   SLOT(onActiveProperties(QString,QVariantMap,QStringList)));
    bus.disconnect(QString::fromLatin1(kNmService), m_activePath, QString::fromLatin1(kActiveIface),
                   QStringLiteral("StateChanged"), this,
                   SLOT(onActiveStateChanged(uint,uint)));
}

void P2PSession::handleActiveState(quint32 state)
{
    if (state == kActiveStateActivated)
        finishActivated();
    else if (state == kActiveStateDeactivated) {
        m_statePoll.stop();
        m_active = false;
        unwatchActiveConnection();
        m_activePath.clear();
        Q_EMIT deactivated();
    }
}

QString P2PSession::queryLocalIpv4() const
{
    if (m_activePath.isEmpty())
        return {};
    const QVariant ip4Var = readProperty(m_activePath,
                                         QString::fromLatin1(kActiveIface),
                                         QStringLiteral("Ip4Config"));
    const auto ip4Path = qdbus_cast<QDBusObjectPath>(ip4Var);
    if (ip4Path.path().isEmpty() || ip4Path.path() == QLatin1String("/"))
        return {};

    const QVariant dataVar = readProperty(ip4Path.path(),
                                          QString::fromLatin1(kIp4Iface),
                                          QStringLiteral("AddressData"));
    const QDBusArgument arg = dataVar.value<QDBusArgument>();
    if (arg.currentType() != QDBusArgument::ArrayType)
        return {};

    arg.beginArray();
    while (!arg.atEnd()) {
        const QVariantMap entry = qdbus_cast<QVariantMap>(arg);
        const QString address = entry.value(QStringLiteral("address")).toString();
        if (!address.isEmpty()) {
            arg.endArray();
            return address;
        }
    }
    arg.endArray();
    return {};
}

void P2PSession::finishActivated()
{
    if (m_active)
        return;
    m_localIpv4 = queryLocalIpv4();
    if (m_localIpv4.isEmpty())
        return;
    m_statePoll.stop();
    m_active = true;
    qInfo() << "P2P group up, local IPv4" << m_localIpv4;
    Q_EMIT activated(m_localIpv4);
}
