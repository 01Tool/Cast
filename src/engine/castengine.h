#pragma once

#include "engine/sinkdevice.h"
#include "session/wfdvideomode.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <memory>

class CaptureBackend;
class GstEncoder;
class P2PDiscovery;
class P2PSession;
class WfdServer;

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
    void bindSession();
    void onPeersChanged();
    void onScanFinished();
    void onP2PActivated(const QString &localIpv4);
    void onPlayRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &mode);
    void failSession(const QString &message);
    void teardownSession();
    SinkDevice sinkById(const QString &id) const;

    SessionState m_state = SessionState::Idle;
    DisplayServer m_displayServer = DisplayServer::Unknown;
    QVector<SinkDevice> m_sinks;
    QString m_statusMessage;
    QString m_selectedSinkId;
    std::unique_ptr<CaptureBackend> m_capture;
    std::unique_ptr<P2PDiscovery> m_discovery;
    std::unique_ptr<P2PSession> m_p2p;
    std::unique_ptr<WfdServer> m_wfd;
    std::unique_ptr<GstEncoder> m_encoder;
    QTimer m_connectTimer;
    bool m_tearingDown = false;
};
