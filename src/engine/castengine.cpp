#include "engine/castengine.h"

#include "capture/portalcapture.h"
#include "capture/x11capture.h"

#include <DGuiApplicationHelper>

#include <QDebug>

DGUI_USE_NAMESPACE

CastEngine::CastEngine(QObject *parent)
    : QObject(parent)
{
    selectCaptureBackend();
    setStatusMessage(QStringLiteral("Idle. Scan to search for Miracast displays."));
}

CastEngine::~CastEngine()
{
    if (m_capture)
        m_capture->stop();
}

CastEngine::SessionState CastEngine::state() const
{
    return m_state;
}

CastEngine::DisplayServer CastEngine::displayServer() const
{
    return m_displayServer;
}

QVector<SinkDevice> CastEngine::sinks() const
{
    return m_sinks;
}

QString CastEngine::statusMessage() const
{
    return m_statusMessage;
}

QString CastEngine::selectedSinkId() const
{
    return m_selectedSinkId;
}

void CastEngine::startScan()
{
    if (m_state == SessionState::Connecting || m_state == SessionState::Streaming) {
        Q_EMIT errorOccurred(QStringLiteral("Stop the current session before scanning."));
        return;
    }

    setState(SessionState::Scanning);
    setStatusMessage(QStringLiteral("Scanning for Miracast sinks…"));

    // NetworkManager P2P discovery is wired in the next cut.
    m_sinks.clear();
    Q_EMIT sinksChanged();

    setStatusMessage(QStringLiteral(
        "No devices yet. Wi-Fi Direct / NetworkManager P2P discovery is not wired."));
    setState(SessionState::Idle);
}

void CastEngine::stopScan()
{
    if (m_state != SessionState::Scanning)
        return;

    setState(SessionState::Idle);
    setStatusMessage(QStringLiteral("Scan stopped."));
}

void CastEngine::connectToSink(const QString &id)
{
    if (id.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral("Select a display first."));
        return;
    }

    bool found = false;
    for (const auto &sink : m_sinks) {
        if (sink.id == id) {
            found = true;
            break;
        }
    }
    if (!found) {
        Q_EMIT errorOccurred(QStringLiteral("Unknown display."));
        return;
    }

    m_selectedSinkId = id;
    setState(SessionState::Connecting);
    setStatusMessage(QStringLiteral("Connecting…"));

    if (!m_capture) {
        setState(SessionState::Failed);
        setStatusMessage(QStringLiteral("No capture backend for this session."));
        Q_EMIT errorOccurred(m_statusMessage);
        return;
    }

    if (!m_capture->start()) {
        const QString err = m_capture->lastError();
        setState(SessionState::Failed);
        setStatusMessage(err);
        Q_EMIT errorOccurred(err);
        return;
    }

    setState(SessionState::Streaming);
    setStatusMessage(QStringLiteral("Mirroring."));
}

void CastEngine::disconnectFromSink()
{
    if (m_capture)
        m_capture->stop();

    m_selectedSinkId.clear();
    setState(SessionState::Stopped);
    setStatusMessage(QStringLiteral("Disconnected."));
    setState(SessionState::Idle);
}

void CastEngine::setState(SessionState state)
{
    if (m_state == state)
        return;
    m_state = state;
    qInfo() << "session state" << static_cast<int>(state);
    Q_EMIT stateChanged(m_state);
}

void CastEngine::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message)
        return;
    m_statusMessage = message;
    Q_EMIT statusMessageChanged(m_statusMessage);
}

void CastEngine::selectCaptureBackend()
{
    auto *helper = DGuiApplicationHelper::instance();

    if (helper->testAttribute(DGuiApplicationHelper::IsWaylandPlatform)) {
        m_displayServer = DisplayServer::Wayland;
        m_capture = std::make_unique<PortalCapture>();
        qInfo() << "display server Wayland, capture" << m_capture->name();
        return;
    }

    if (helper->testAttribute(DGuiApplicationHelper::IsXWindowPlatform)) {
        m_displayServer = DisplayServer::X11;
        m_capture = std::make_unique<X11Capture>();
        qInfo() << "display server X11, capture" << m_capture->name();
        return;
    }

    m_displayServer = DisplayServer::Unknown;
    qWarning() << "unknown display server, no capture backend";
}
