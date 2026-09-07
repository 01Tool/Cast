#include "dbus/castdbusservice.h"

#include "dbus/castdbus.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusReply>
#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <unistd.h>

CastDBusService::CastDBusService(CastEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    connect(m_engine, &CastEngine::stateChanged, this, [this](CastEngine::SessionState state) {
        Q_EMIT stateChanged(stateName(state));
        Q_EMIT selectedSinkIdChanged(m_engine->selectedSinkId());
    });
    connect(m_engine, &CastEngine::statusMessageChanged, this, &CastDBusService::statusMessageChanged);
    connect(m_engine, &CastEngine::sinksChanged, this, &CastDBusService::sinksChanged);
    connect(m_engine, &CastEngine::errorOccurred, this, &CastDBusService::errorOccurred);
    connect(m_engine, &CastEngine::pairingRequested, this, [this](CastEngine::PairingKind, const QString &) {
        Q_EMIT pairingRequested();
        Q_EMIT raiseRequested();
    });
}

QString CastDBusService::state() const
{
    return stateName(m_engine->state());
}

QString CastDBusService::statusMessage() const
{
    return m_engine->statusMessage();
}

QString CastDBusService::selectedSinkId() const
{
    return m_engine->selectedSinkId();
}

bool CastDBusService::registerService()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return false;
    if (!bus.registerService(QString::fromLatin1(CastDBus::service)))
        return false;
    return bus.registerObject(QString::fromLatin1(CastDBus::path), this,
                              QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals
                                  | QDBusConnection::ExportAllProperties);
}

bool CastDBusService::authorize() const
{
    if (!calledFromDBus())
        return true;

    auto *iface = connection().interface();
    if (!iface) {
        sendErrorReply(QDBusError::AccessDenied,
                       QStringLiteral("No bus interface to identify the caller"));
        return false;
    }

    const QString peer = message().service();
    const QDBusReply<uint> uidReply = iface->serviceUid(peer);
    if (uidReply.isValid() && uidReply.value() != static_cast<uint>(::getuid())) {
        qWarning() << "denied D-Bus" << message().member() << "from uid" << uidReply.value();
        sendErrorReply(QDBusError::AccessDenied,
                       QStringLiteral("Caller is not the session user"));
        return false;
    }

    const QDBusReply<uint> pidReply = iface->servicePid(peer);
    const QString exe = pidReply.isValid() ? CastDBus::peerExecutable(pidReply.value()) : QString();
    const QString self = QCoreApplication::applicationFilePath();
    if (CastDBus::controlCallerAllowed(exe, self))
        return true;

    qWarning() << "denied D-Bus" << message().member() << "from" << peer << "pid"
               << (pidReply.isValid() ? pidReply.value() : 0) << exe;
    sendErrorReply(QDBusError::AccessDenied,
                   QStringLiteral("Only ot-cast and the DDE tray host may call %1")
                       .arg(message().member()));
    return false;
}

void CastDBusService::StartScan()
{
    if (!authorize())
        return;
    m_engine->startScan();
}

void CastDBusService::StopScan()
{
    if (!authorize())
        return;
    m_engine->stopScan();
}

void CastDBusService::Connect(const QString &sinkId)
{
    if (!authorize())
        return;
    m_engine->connectToSink(sinkId);
}

void CastDBusService::Disconnect()
{
    if (!authorize())
        return;
    m_engine->disconnectFromSink();
}

void CastDBusService::RaiseWindow()
{
    if (!authorize())
        return;
    Q_EMIT raiseRequested();
}

QString CastDBusService::SinksJson() const
{
    if (!authorize())
        return {};
    QJsonArray rows;
    for (const SinkDevice &sink : m_engine->sinks()) {
        QJsonObject row;
        row.insert(QStringLiteral("id"), sink.id);
        row.insert(QStringLiteral("name"), sink.name);
        row.insert(QStringLiteral("protocol"),
                   sink.protocol == CastProtocol::Dlna ? QStringLiteral("DLNA")
                                                       : QStringLiteral("Miracast"));
        row.insert(QStringLiteral("address"),
                   sink.miceHost.isEmpty() ? sink.address : sink.miceHost);
        row.insert(QStringLiteral("mice"), sink.miceCapable);
        rows.append(row);
    }
    return QString::fromUtf8(QJsonDocument(rows).toJson(QJsonDocument::Compact));
}

QString CastDBusService::stateName(CastEngine::SessionState state)
{
    switch (state) {
    case CastEngine::SessionState::Scanning:
        return QStringLiteral("Scanning");
    case CastEngine::SessionState::Connecting:
        return QStringLiteral("Connecting");
    case CastEngine::SessionState::Streaming:
        return QStringLiteral("Streaming");
    case CastEngine::SessionState::Failed:
        return QStringLiteral("Failed");
    case CastEngine::SessionState::Stopped:
        return QStringLiteral("Stopped");
    case CastEngine::SessionState::Idle:
        break;
    }
    return QStringLiteral("Idle");
}