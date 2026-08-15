#include "session/nmsecretagent.h"

#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDebug>
#include <QVariantMap>

namespace {

constexpr auto kNmService = "org.freedesktop.NetworkManager";
constexpr auto kAgentManagerPath = "/org/freedesktop/NetworkManager/AgentManager";
constexpr auto kAgentManagerIface = "org.freedesktop.NetworkManager.AgentManager";
constexpr auto kAgentPath = "/org/freedesktop/NetworkManager/SecretAgent";
constexpr auto kAgentIface = "org.freedesktop.NetworkManager.SecretAgent";
constexpr auto kAgentId = "org.deepin.miracast";

constexpr auto kErrUserCanceled = "org.freedesktop.NetworkManager.SecretAgent.Error.UserCanceled";
constexpr auto kErrAgentCanceled = "org.freedesktop.NetworkManager.SecretAgent.Error.AgentCanceled";
constexpr auto kErrNoSecrets = "org.freedesktop.NetworkManager.SecretAgent.Error.NoSecrets";

// NMSecretAgentGetSecretsFlags
constexpr uint kAllowInteraction = 0x1;
constexpr uint kWpsPbcActive = 0x8;

using ConnectionMap = QMap<QString, QVariantMap>;

const char kIntrospection[] =
    "<interface name=\"org.freedesktop.NetworkManager.SecretAgent\">"
    "  <method name=\"GetSecrets\">"
    "    <arg name=\"connection\" type=\"a{sa{sv}}\" direction=\"in\"/>"
    "    <arg name=\"connection_path\" type=\"o\" direction=\"in\"/>"
    "    <arg name=\"setting_name\" type=\"s\" direction=\"in\"/>"
    "    <arg name=\"hints\" type=\"as\" direction=\"in\"/>"
    "    <arg name=\"flags\" type=\"u\" direction=\"in\"/>"
    "    <arg name=\"secrets\" type=\"a{sa{sv}}\" direction=\"out\"/>"
    "  </method>"
    "  <method name=\"CancelGetSecrets\">"
    "    <arg name=\"connection_path\" type=\"o\" direction=\"in\"/>"
    "    <arg name=\"setting_name\" type=\"s\" direction=\"in\"/>"
    "  </method>"
    "  <method name=\"SaveSecrets\">"
    "    <arg name=\"connection\" type=\"a{sa{sv}}\" direction=\"in\"/>"
    "    <arg name=\"connection_path\" type=\"o\" direction=\"in\"/>"
    "  </method>"
    "  <method name=\"DeleteSecrets\">"
    "    <arg name=\"connection\" type=\"a{sa{sv}}\" direction=\"in\"/>"
    "    <arg name=\"connection_path\" type=\"o\" direction=\"in\"/>"
    "  </method>"
    "</interface>";

QString connectionType(const ConnectionMap &connection)
{
    return connection.value(QStringLiteral("connection")).value(QStringLiteral("type")).toString();
}

QString connectionId(const ConnectionMap &connection)
{
    return connection.value(QStringLiteral("connection")).value(QStringLiteral("id")).toString();
}

bool isOurP2P(const ConnectionMap &connection)
{
    if (connectionType(connection) == QLatin1String("wifi-p2p"))
        return true;
    return connectionId(connection).startsWith(QLatin1String("Miracast "));
}

} // namespace

NmSecretAgent::NmSecretAgent(QObject *parent)
    : QDBusVirtualObject(parent)
    , m_bus(QDBusConnection::systemBus())
{
    qDBusRegisterMetaType<ConnectionMap>();

    if (!m_bus.isConnected()) {
        qWarning() << "System D-Bus unavailable, pairing prompts disabled";
        return;
    }

    if (!m_bus.registerVirtualObject(QString::fromLatin1(kAgentPath), this)) {
        qWarning() << "Could not export NM SecretAgent" << m_bus.lastError().message();
        return;
    }

    QDBusInterface manager(QString::fromLatin1(kNmService),
                           QString::fromLatin1(kAgentManagerPath),
                           QString::fromLatin1(kAgentManagerIface),
                           m_bus);
    const QDBusMessage reply = manager.call(QStringLiteral("Register"),
                                            QString::fromLatin1(kAgentId));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "NM AgentManager.Register failed" << reply.errorMessage();
        m_bus.unregisterObject(QString::fromLatin1(kAgentPath));
        return;
    }

    m_registered = true;
    qInfo() << "Registered NM SecretAgent" << kAgentId;
}

NmSecretAgent::~NmSecretAgent()
{
    cancel();
    unregisterAgent();
}

bool NmSecretAgent::registered() const
{
    return m_registered;
}

bool NmSecretAgent::pairingPending() const
{
    return m_hasPending;
}

QString NmSecretAgent::introspect(const QString &path) const
{
    Q_UNUSED(path);
    return QString::fromLatin1(kIntrospection);
}

bool NmSecretAgent::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    if (message.interface() != QLatin1String(kAgentIface))
        return false;

    if (message.member() == QLatin1String("GetSecrets"))
        return handleGetSecrets(message, connection);

    if (message.member() == QLatin1String("CancelGetSecrets")) {
        if (m_hasPending) {
            replyPendingError(QString::fromLatin1(kErrAgentCanceled),
                              QStringLiteral("NetworkManager cancelled the secret request."));
        }
        connection.send(message.createReply());
        return true;
    }

    if (message.member() == QLatin1String("SaveSecrets")
        || message.member() == QLatin1String("DeleteSecrets")) {
        connection.send(message.createReply());
        return true;
    }

    return false;
}

bool NmSecretAgent::handleGetSecrets(const QDBusMessage &message, const QDBusConnection &connection)
{
    const QList<QVariant> args = message.arguments();
    if (args.size() < 5) {
        connection.send(message.createErrorReply(QDBusError::InvalidArgs, QStringLiteral("GetSecrets")));
        return true;
    }

    ConnectionMap connectionMap;
    if (args.at(0).canConvert<QDBusArgument>()) {
        const QDBusArgument arg = args.at(0).value<QDBusArgument>();
        connectionMap = qdbus_cast<ConnectionMap>(arg);
    } else if (args.at(0).canConvert<ConnectionMap>()) {
        connectionMap = qvariant_cast<ConnectionMap>(args.at(0));
    }

    const QString settingName = args.at(2).toString();
    const QStringList hints = args.at(3).toStringList();
    const uint flags = args.at(4).toUInt();

    if (!isOurP2P(connectionMap)) {
        connection.send(message.createErrorReply(QString::fromLatin1(kErrNoSecrets),
                                                 QStringLiteral("Not a Miracast P2P connection.")));
        return true;
    }

    if ((flags & kAllowInteraction) == 0 && (flags & kWpsPbcActive) == 0) {
        connection.send(message.createErrorReply(QString::fromLatin1(kErrNoSecrets),
                                                 QStringLiteral("No stored WPS PIN.")));
        return true;
    }

    if (m_hasPending)
        replyPendingError(QString::fromLatin1(kErrAgentCanceled), QStringLiteral("Replaced by a new request."));

    m_pending = message;
    m_hasPending = true;

    QString sinkName = connectionId(connectionMap);
    const QString peer = connectionMap.value(QStringLiteral("wifi-p2p"))
                             .value(QStringLiteral("peer"))
                             .toString();
    if (sinkName.startsWith(QLatin1String("Miracast ")))
        sinkName = sinkName.mid(9);
    if (sinkName.isEmpty())
        sinkName = peer;

    const bool wantPin = hints.contains(QStringLiteral("pin"), Qt::CaseInsensitive)
        || settingName.contains(QStringLiteral("security"), Qt::CaseInsensitive)
        || ((flags & kWpsPbcActive) == 0);
    const PairingKind kind = (flags & kWpsPbcActive) && !hints.contains(QStringLiteral("pin"))
        ? PairingKind::PushButton
        : (wantPin ? PairingKind::Pin : PairingKind::PushButton);

    qInfo() << "NM pairing request" << int(kind) << sinkName << "hints" << hints << "flags" << flags;
    Q_EMIT pairingRequested(kind, sinkName);
    return true;
}

void NmSecretAgent::providePin(const QString &pin)
{
    if (!m_hasPending)
        return;

    const QString trimmed = pin.trimmed();
    ConnectionMap secrets;
    QVariantMap security;
    security.insert(QStringLiteral("pin"), trimmed);
    secrets.insert(QStringLiteral("802-11-wireless-security"), security);

    const QDBusMessage reply = m_pending.createReply(QVariant::fromValue(secrets));
    m_bus.send(reply);
    m_hasPending = false;
    m_pending = QDBusMessage();
    Q_EMIT pairingFinished();
}

void NmSecretAgent::cancel()
{
    if (!m_hasPending)
        return;
    replyPendingError(QString::fromLatin1(kErrUserCanceled),
                      QStringLiteral("User cancelled pairing."));
}

void NmSecretAgent::unregisterAgent()
{
    if (!m_registered)
        return;
    QDBusInterface manager(QString::fromLatin1(kNmService),
                           QString::fromLatin1(kAgentManagerPath),
                           QString::fromLatin1(kAgentManagerIface),
                           m_bus);
    manager.call(QStringLiteral("Unregister"));
    m_bus.unregisterObject(QString::fromLatin1(kAgentPath));
    m_registered = false;
}

void NmSecretAgent::replyPendingError(const QString &name, const QString &message)
{
    if (!m_hasPending)
        return;
    m_bus.send(m_pending.createErrorReply(name, message));
    m_hasPending = false;
    m_pending = QDBusMessage();
    Q_EMIT pairingFinished();
}
