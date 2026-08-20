#include "tray/castclient.h"

#include "dbus/castdbus.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

CastClient::CastClient(QObject *parent)
    : QObject(parent)
{
    m_watcher = new QDBusServiceWatcher(QString::fromLatin1(CastDBus::service),
                                        QDBusConnection::sessionBus(),
                                        QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &newOwner) {
                if (newOwner.isEmpty()) {
                    m_iface.reset();
                    m_available = false;
                    Q_EMIT availableChanged();
                    return;
                }
                bindInterface();
            });
    if (QDBusConnection::sessionBus().interface()
        && QDBusConnection::sessionBus().interface()->isServiceRegistered(
            QString::fromLatin1(CastDBus::service))) {
        bindInterface();
    }
}

bool CastClient::available() const
{
    return m_available;
}

QString CastClient::state() const
{
    return m_state;
}

QString CastClient::statusMessage() const
{
    return m_status;
}

QJsonArray CastClient::sinks() const
{
    return m_sinks;
}

void CastClient::ensureService()
{
    if (m_available)
        return;
    QProcess::startDetached(QStringLiteral("ot-cast"), {QStringLiteral("--background")});
    QTimer::singleShot(800, this, [this]() {
        if (!m_available)
            bindInterface();
    });
}

void CastClient::startScan()
{
    ensureService();
    if (m_iface)
        m_iface->asyncCall(QStringLiteral("StartScan"));
}

void CastClient::connectToSink(const QString &id)
{
    ensureService();
    if (m_iface)
        m_iface->asyncCall(QStringLiteral("Connect"), id);
}

void CastClient::disconnectFromSink()
{
    if (m_iface)
        m_iface->asyncCall(QStringLiteral("Disconnect"));
}

void CastClient::raiseWindow()
{
    ensureService();
    if (m_iface)
        m_iface->asyncCall(QStringLiteral("RaiseWindow"));
    else
        QProcess::startDetached(QStringLiteral("ot-cast"), QStringList());
}

void CastClient::bindInterface()
{
    m_iface = std::make_unique<QDBusInterface>(QString::fromLatin1(CastDBus::service),
                                               QString::fromLatin1(CastDBus::path),
                                               QString::fromLatin1(CastDBus::interface),
                                               QDBusConnection::sessionBus());
    if (!m_iface->isValid()) {
        m_available = false;
        Q_EMIT availableChanged();
        return;
    }

    const auto svc = QString::fromLatin1(CastDBus::service);
    const auto path = QString::fromLatin1(CastDBus::path);
    const auto iface = QString::fromLatin1(CastDBus::interface);
    auto bus = QDBusConnection::sessionBus();
    bus.connect(svc, path, iface, QStringLiteral("sinksChanged"), this, SLOT(onRemoteChanged()));
    bus.connect(svc, path, iface, QStringLiteral("stateChanged"), this, SLOT(onRemoteChanged()));
    bus.connect(svc, path, iface, QStringLiteral("statusMessageChanged"), this,
                SLOT(onRemoteChanged()));
    bus.connect(svc, path, iface, QStringLiteral("errorOccurred"), this,
                SIGNAL(errorOccurred(QString)));
    bus.connect(svc, path, iface, QStringLiteral("pairingRequested"), this, SLOT(raiseWindow()));

    m_available = true;
    Q_EMIT availableChanged();
    refresh();
}

void CastClient::onRemoteChanged()
{
    refresh();
}

void CastClient::refresh()
{
    if (!m_iface || !m_iface->isValid())
        return;
    m_state = m_iface->property("State").toString();
    m_status = m_iface->property("StatusMessage").toString();
    const auto reply = m_iface->call(QStringLiteral("SinksJson"));
    if (reply.arguments().isEmpty())
        m_sinks = {};
    else {
        const QJsonDocument doc =
            QJsonDocument::fromJson(reply.arguments().constFirst().toString().toUtf8());
        m_sinks = doc.array();
    }
    Q_EMIT stateChanged();
    Q_EMIT sinksChanged();
}