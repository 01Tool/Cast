#pragma once

#include "engine/sinkdevice.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

class CaptureBackend;
class P2PDiscovery;

class CastEngine : public QObject
{
    Q_OBJECT

public:
    enum class SessionState {
        Idle,
        Scanning,
        Connecting,
        Streaming,
        Failed,
        Stopped,
    };
    Q_ENUM(SessionState)

    enum class DisplayServer {
        X11,
        Wayland,
        Unknown,
    };
    Q_ENUM(DisplayServer)

    explicit CastEngine(QObject *parent = nullptr);
    ~CastEngine() override;

    SessionState state() const;
    DisplayServer displayServer() const;
    QVector<SinkDevice> sinks() const;
    QString statusMessage() const;
    QString selectedSinkId() const;

public Q_SLOTS:
    void startScan();
    void stopScan();
    void connectToSink(const QString &id);
    void disconnectFromSink();

Q_SIGNALS:
    void stateChanged(CastEngine::SessionState state);
    void sinksChanged();
    void statusMessageChanged(const QString &message);
    void errorOccurred(const QString &message);

private:
    void setState(SessionState state);
    void setStatusMessage(const QString &message);
    void selectCaptureBackend();
    void bindDiscovery();
    void onPeersChanged();
    void onScanFinished();

    SessionState m_state = SessionState::Idle;
    DisplayServer m_displayServer = DisplayServer::Unknown;
    QVector<SinkDevice> m_sinks;
    QString m_statusMessage;
    QString m_selectedSinkId;
    std::unique_ptr<CaptureBackend> m_capture;
    std::unique_ptr<P2PDiscovery> m_discovery;
};
