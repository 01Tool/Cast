#include "dbus/castdbusservice.h"

#include "dbus/castdbus.h"

#include <QDBusConnection>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

void CastDBusService::StartScan()
{
    m_engine->startScan();
}

void CastDBusService::StopScan()
{
    m_engine->stopScan();
}

void CastDBusService::Connect(const QString &sinkId)
{
    m_engine->connectToSink(sinkId);
}

void CastDBusService::Disconnect()
{
    m_engine->disconnectFromSink();
}

void CastDBusService::RaiseWindow()
{
    Q_EMIT raiseRequested();
}

QString CastDBusService::SinksJson() const
{
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