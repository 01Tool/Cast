#include "engine/castengine.h"

#include "capture/portalcapture.h"
#include "capture/x11capture.h"
#include "discovery/p2pdiscovery.h"
#include "session/gstencoder.h"
#include "session/nmsecretagent.h"
#include "session/p2psession.h"
#include "session/wfdserver.h"

#include <DGuiApplicationHelper>

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

DGUI_USE_NAMESPACE

CastEngine::CastEngine(QObject *parent)
    : QObject(parent)
{
    selectCaptureBackend();
    bindDiscovery();
    bindSession();
    bindPairing();
    watchScreens();
    refreshDisplays();
    setStatusMessage(QStringLiteral("Idle. Scan to search for Miracast displays."));
}

CastEngine::~CastEngine()
{
    teardownSession();
    if (m_discovery)
        m_discovery->stopScan();
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

bool CastEngine::audioEnabled() const
{
    return m_audioEnabled;
}

void CastEngine::setAudioEnabled(bool enabled)
{
    if (m_audioEnabled == enabled)
        return;
    m_audioEnabled = enabled;
    qInfo() << "audio enabled" << enabled;
    Q_EMIT audioEnabledChanged(m_audioEnabled);
}

QVector<DisplaySource> CastEngine::displays() const
{
    return m_displays;
}

QString CastEngine::selectedDisplayId() const
{
    return m_selectedDisplayId;
}

DisplaySource CastEngine::selectedDisplay() const
{
    const DisplaySource chosen = displayById(m_selectedDisplayId);
    if (chosen.isValid())
        return chosen;
    return primaryDisplay();
}

void CastEngine::submitPairingPin(const QString &pin)
{
    if (m_secrets)
        m_secrets->providePin(pin);
}

void CastEngine::cancelPairing()
{
    if (m_secrets)
        m_secrets->cancel();
}

void CastEngine::bindPairing()
{
    m_secrets = std::make_unique<NmSecretAgent>();
    connect(m_secrets.get(), &NmSecretAgent::pairingRequested, this,
            [this](NmSecretAgent::PairingKind kind, const QString &sinkName) {
                const PairingKind uiKind = (kind == NmSecretAgent::PairingKind::Pin)
                    ? PairingKind::Pin
                    : PairingKind::PushButton;
                if (uiKind == PairingKind::Pin)
                    setStatusMessage(QStringLiteral("Enter the pairing PIN for %1…").arg(sinkName));
                else
                    setStatusMessage(QStringLiteral("Confirm pairing on %1…").arg(sinkName));
                Q_EMIT pairingRequested(uiKind, sinkName);
            });
    connect(m_secrets.get(), &NmSecretAgent::pairingFinished, this, [this]() {
        Q_EMIT pairingFinished();
    });
}

void CastEngine::setSelectedDisplayId(const QString &id)
{
    const DisplaySource source = displayById(id);
    const QString resolved = source.isValid() ? source.id : primaryDisplay().id;
    if (m_selectedDisplayId == resolved)
        return;
    m_selectedDisplayId = resolved;
    qInfo() << "selected monitor" << m_selectedDisplayId;
    Q_EMIT selectedDisplayChanged(m_selectedDisplayId);
}

void CastEngine::startScan()
{
    if (m_state == SessionState::Connecting || m_state == SessionState::Streaming) {
        Q_EMIT errorOccurred(QStringLiteral("Stop the current session before scanning."));
        return;
    }

    setState(SessionState::Scanning);
    setStatusMessage(QStringLiteral("Scanning for Miracast displays…"));
    m_discovery->startScan(P2PDiscovery::DefaultScanSeconds);
}

void CastEngine::stopScan()
{
    if (m_state != SessionState::Scanning)
        return;

    m_discovery->stopScan();
}

void CastEngine::connectToSink(const QString &id)
{
    if (id.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral("Select a display first."));
        return;
    }

    const SinkDevice sink = sinkById(id);
    if (sink.id.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral("Unknown display."));
        return;
    }

    m_selectedSinkId = id;
    setState(SessionState::Connecting);
    setStatusMessage(QStringLiteral("Connecting…"));
    if (m_discovery && m_discovery->scanning())
        m_discovery->stopScan();

    if (!m_capture) {
        failSession(QStringLiteral("No capture backend for this session."));
        return;
    }

    m_sessionSource = selectedDisplay();
    if (!m_capture->start(m_sessionSource)) {
        failSession(m_capture->lastError());
        return;
    }

    m_connectTimer.start(30000);
    m_p2p->activate(sink);
}

void CastEngine::disconnectFromSink()
{
    teardownSession();
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

void CastEngine::bindDiscovery()
{
    m_discovery = std::make_unique<P2PDiscovery>();
    connect(m_discovery.get(), &P2PDiscovery::peersChanged, this, &CastEngine::onPeersChanged);
    connect(m_discovery.get(), &P2PDiscovery::scanFinished, this, &CastEngine::onScanFinished);
    connect(m_discovery.get(), &P2PDiscovery::errorOccurred, this, &CastEngine::errorOccurred);
    connect(m_discovery.get(), &P2PDiscovery::statusChanged, this, &CastEngine::setStatusMessage);
}

void CastEngine::onPeersChanged()
{
    m_sinks = m_discovery->peers();
    Q_EMIT sinksChanged();

    if (m_state != SessionState::Scanning)
        return;

    int wfd = 0;
    for (const auto &sink : m_sinks) {
        if (sink.wfdCapable)
            ++wfd;
    }
    if (m_sinks.isEmpty()) {
        setStatusMessage(QStringLiteral("Scanning for Miracast displays…"));
        return;
    }
    setStatusMessage(QStringLiteral("Found %1 device(s) (%2 with WFD IEs).")
                         .arg(m_sinks.size())
                         .arg(wfd));
}

void CastEngine::onScanFinished()
{
    if (m_state == SessionState::Connecting || m_state == SessionState::Streaming)
        return;

    m_sinks = m_discovery->peers();
    Q_EMIT sinksChanged();

    if (m_sinks.isEmpty()) {
        setStatusMessage(QStringLiteral(
            "No P2P devices found. The sink must be in wireless-display / Miracast mode."));
        setState(SessionState::Idle);
        return;
    }

    int wfd = 0;
    for (const auto &sink : m_sinks) {
        if (sink.wfdCapable)
            ++wfd;
    }
    if (wfd == 0) {
        setStatusMessage(QStringLiteral(
            "Found %1 P2P device(s) with no WFD IEs. "
            "wpa_supplicant may lack CONFIG_WIFI_DISPLAY, or they are not Miracast sinks.")
                             .arg(m_sinks.size()));
    } else {
        setStatusMessage(QStringLiteral("Scan finished. %1 Miracast display(s) available.")
                             .arg(wfd));
    }
    setState(SessionState::Idle);
}

void CastEngine::bindSession()
{
    m_p2p = std::make_unique<P2PSession>();
    m_wfd = std::make_unique<WfdServer>();
    m_encoder = std::make_unique<GstEncoder>();

    m_connectTimer.setSingleShot(true);
    connect(&m_connectTimer, &QTimer::timeout, this, [this]() {
        if (m_state == SessionState::Connecting)
            failSession(QStringLiteral("Timed out forming the Wi-Fi Direct group or WFD session."));
    });

    connect(m_p2p.get(), &P2PSession::statusChanged, this, &CastEngine::setStatusMessage);
    connect(m_p2p.get(), &P2PSession::activated, this, &CastEngine::onP2PActivated);
    connect(m_p2p.get(), &P2PSession::failed, this, &CastEngine::failSession);
    connect(m_p2p.get(), &P2PSession::deactivated, this, [this]() {
        if (!m_tearingDown
            && (m_state == SessionState::Streaming || m_state == SessionState::Connecting))
            failSession(QStringLiteral("Wi-Fi Direct group dropped."));
    });

    connect(m_wfd.get(), &WfdServer::statusChanged, this, &CastEngine::setStatusMessage);
    connect(m_wfd.get(), &WfdServer::failed, this, &CastEngine::failSession);
    connect(m_wfd.get(), &WfdServer::playRequested, this, &CastEngine::onPlayRequested);

    connect(m_encoder.get(), &GstEncoder::started, this, [this]() {
        m_connectTimer.stop();
        setState(SessionState::Streaming);
        setStatusMessage(QStringLiteral("Mirroring %1.").arg(m_encoder->streamDescription()));
    });
    connect(m_encoder.get(), &GstEncoder::failed, this, &CastEngine::failSession);
}

void CastEngine::onP2PActivated(const QString &localIpv4)
{
    if (m_state != SessionState::Connecting)
        return;
    if (!m_wfd->listen(localIpv4, m_audioEnabled))
        return;
}

void CastEngine::onPlayRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
                                const WfdAudioMode &audio)
{
    if (m_state != SessionState::Connecting && m_state != SessionState::Streaming)
        return;
    setStatusMessage(QStringLiteral("Starting X11 encoder (%1, %2, %3)…")
                         .arg(video.description(), audio.description(),
                              m_sessionSource.shortName()));
    m_encoder->start(sinkIp, rtpPort, video, audio, m_sessionSource);
}

void CastEngine::failSession(const QString &message)
{
    teardownSession();
    setState(SessionState::Failed);
    setStatusMessage(message);
    Q_EMIT errorOccurred(message);
    setState(SessionState::Idle);
}

void CastEngine::teardownSession()
{
    if (m_tearingDown)
        return;
    m_tearingDown = true;
    cancelPairing();
    m_connectTimer.stop();
    if (m_encoder)
        m_encoder->stop();
    if (m_wfd)
        m_wfd->stop();
    if (m_p2p)
        m_p2p->deactivate();
    if (m_capture)
        m_capture->stop();
    m_tearingDown = false;
}

SinkDevice CastEngine::sinkById(const QString &id) const
{
    for (const auto &sink : m_sinks) {
        if (sink.id == id)
            return sink;
    }
    return {};
}

DisplaySource CastEngine::displayById(const QString &id) const
{
    if (id.isEmpty())
        return {};
    for (const auto &display : m_displays) {
        if (display.id == id)
            return display;
    }
    return {};
}

DisplaySource CastEngine::primaryDisplay() const
{
    for (const auto &display : m_displays) {
        if (display.primary)
            return display;
    }
    return m_displays.value(0);
}

void CastEngine::watchScreens()
{
    auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    if (!app)
        return;

    connect(app, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, &CastEngine::refreshDisplays,
                Qt::UniqueConnection);
        refreshDisplays();
    });
    connect(app, &QGuiApplication::screenRemoved, this, &CastEngine::refreshDisplays);
    connect(app, &QGuiApplication::primaryScreenChanged, this, &CastEngine::refreshDisplays);
    for (QScreen *screen : app->screens()) {
        connect(screen, &QScreen::geometryChanged, this, &CastEngine::refreshDisplays,
                Qt::UniqueConnection);
    }
}

void CastEngine::refreshDisplays()
{
    auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    QVector<DisplaySource> next;
    if (app) {
        QScreen *primary = app->primaryScreen();
        int index = 0;
        for (QScreen *screen : app->screens()) {
            DisplaySource source;
            source.id = screen->name();
            if (source.id.isEmpty())
                source.id = QStringLiteral("screen-%1").arg(index);
            QString title = screen->name();
            const QString model =
                (screen->manufacturer() + QLatin1Char(' ') + screen->model()).trimmed();
            if (!model.isEmpty())
                title = model;
            if (title.isEmpty())
                title = source.id;
            const QRect g = screen->geometry();
            source.x = g.x();
            source.y = g.y();
            source.width = g.width();
            source.height = g.height();
            source.primary = (screen == primary);
            source.name = QStringLiteral("%1 (%2×%3)%4")
                              .arg(title)
                              .arg(source.width)
                              .arg(source.height)
                              .arg(source.primary ? QStringLiteral(" · primary") : QString());
            next.append(source);
            ++index;
        }
    }

    m_displays = next;
    const bool sessionLocked =
        m_state == SessionState::Connecting || m_state == SessionState::Streaming;
    if (!sessionLocked) {
        if (!displayById(m_selectedDisplayId).isValid()) {
            const DisplaySource primary = primaryDisplay();
            if (m_selectedDisplayId != primary.id) {
                m_selectedDisplayId = primary.id;
                Q_EMIT selectedDisplayChanged(m_selectedDisplayId);
            }
        }
    }
    Q_EMIT displaysChanged();
}
