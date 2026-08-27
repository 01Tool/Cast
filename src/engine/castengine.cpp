#include "engine/castengine.h"

#include "capture/portalcapture.h"
#include "capture/x11capture.h"
#include "discovery/dlnadiscovery.h"
#include "discovery/micediscovery.h"
#include "discovery/p2pdiscovery.h"
#include "session/dlnasession.h"
#include "session/gstencoder.h"
#include "session/micesession.h"
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
    setStatusMessage(tr("Idle. Scan to search for Miracast and DLNA displays."));
}

CastEngine::~CastEngine()
{
    teardownSession();
    if (m_discovery)
        m_discovery->stopScan();
    if (m_dlnaDiscovery)
        m_dlnaDiscovery->stopScan();
    if (m_miceDiscovery)
        m_miceDiscovery->stopScan();
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
                    setStatusMessage(tr("Enter the pairing PIN for %1…").arg(sinkName));
                else
                    setStatusMessage(tr("Confirm pairing on %1…").arg(sinkName));
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
        Q_EMIT errorOccurred(tr("Stop the current session before scanning."));
        return;
    }

    m_p2pScanDone = false;
    m_dlnaScanDone = false;
    m_miceScanDone = false;
    setState(SessionState::Scanning);
    setStatusMessage(tr("Scanning for Miracast and DLNA displays…"));
    if (m_discovery)
        m_discovery->startScan(P2PDiscovery::DefaultScanSeconds);
    if (m_dlnaDiscovery)
        m_dlnaDiscovery->startScan(DlnaDiscovery::DefaultScanSeconds);
    if (m_miceDiscovery)
        m_miceDiscovery->startScan(MiceDiscovery::DefaultScanSeconds);
}

void CastEngine::stopScan()
{
    if (m_state != SessionState::Scanning)
        return;

    if (m_discovery)
        m_discovery->stopScan();
    if (m_dlnaDiscovery)
        m_dlnaDiscovery->stopScan();
    if (m_miceDiscovery)
        m_miceDiscovery->stopScan();
}

void CastEngine::connectToSink(const QString &id)
{
    if (id.isEmpty()) {
        Q_EMIT errorOccurred(tr("Select a display first."));
        return;
    }

    const SinkDevice sink = sinkById(id);
    if (sink.id.isEmpty()) {
        Q_EMIT errorOccurred(tr("Unknown display."));
        return;
    }

    m_selectedSinkId = id;
    setState(SessionState::Connecting);
    setStatusMessage(tr("Connecting…"));
    if (m_discovery && m_discovery->scanning())
        m_discovery->stopScan();
    if (m_dlnaDiscovery && m_dlnaDiscovery->scanning())
        m_dlnaDiscovery->stopScan();
    if (m_miceDiscovery && m_miceDiscovery->scanning())
        m_miceDiscovery->stopScan();

    if (!m_capture) {
        failSession(tr("No capture backend for this session."));
        return;
    }

    m_sessionSource = selectedDisplay();
    if (!m_capture->start(m_sessionSource)) {
        failSession(m_capture->lastError());
        return;
    }

    if (sink.protocol == CastProtocol::Dlna)
        connectDlna(sink);
    else
        connectMiracast(sink);
}

void CastEngine::connectMiracast(const SinkDevice &sink)
{
    m_tryingMice = true;
    m_connectTimer.start(20000);
    setStatusMessage(tr("Trying LAN Miracast (same Wi-Fi as the display)…"));
    m_mice->start(sink);
}

void CastEngine::fallbackToP2p(const QString &why)
{
    if (!m_tryingMice)
        return;
    qInfo() << "MICE fallback to P2P" << why;

    const SinkDevice sink = sinkById(m_selectedSinkId);
    if (sink.p2pDevicePath.isEmpty()) {
        failSession(why);
        return;
    }

    m_tryingMice = false;
    if (m_mice)
        m_mice->stop();
    if (m_wfd)
        m_wfd->stop();
    setStatusMessage(tr("LAN Miracast did not start. Trying Wi-Fi Direct…"));
    m_connectTimer.start(90000);
    m_p2p->activate(sink);
}

void CastEngine::connectDlna(const SinkDevice &sink)
{
    m_connectTimer.start(45000);
    m_dlna->start(sink, m_sessionSource, m_audioEnabled, m_encoder.get());
}

void CastEngine::disconnectFromSink()
{
    teardownSession();
    m_selectedSinkId.clear();
    setState(SessionState::Stopped);
    setStatusMessage(tr("Disconnected."));
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
    m_dlnaDiscovery = std::make_unique<DlnaDiscovery>();
    m_miceDiscovery = std::make_unique<MiceDiscovery>();
    connect(m_discovery.get(), &P2PDiscovery::peersChanged, this, &CastEngine::onPeersChanged);
    connect(m_discovery.get(), &P2PDiscovery::scanFinished, this, &CastEngine::onScanFinished);
    connect(m_discovery.get(), &P2PDiscovery::errorOccurred, this, &CastEngine::errorOccurred);
    connect(m_dlnaDiscovery.get(), &DlnaDiscovery::renderersChanged, this,
            &CastEngine::onPeersChanged);
    connect(m_dlnaDiscovery.get(), &DlnaDiscovery::scanFinished, this,
            &CastEngine::onDlnaScanFinished);
    connect(m_dlnaDiscovery.get(), &DlnaDiscovery::errorOccurred, this, &CastEngine::errorOccurred);
    connect(m_miceDiscovery.get(), &MiceDiscovery::sinksChanged, this, &CastEngine::onPeersChanged);
    connect(m_miceDiscovery.get(), &MiceDiscovery::scanFinished, this,
            &CastEngine::onMiceScanFinished);
}

void CastEngine::mergeSinks()
{
    QVector<SinkDevice> p2p = m_discovery ? m_discovery->peers() : QVector<SinkDevice>{};
    const QVector<SinkDevice> mice = m_miceDiscovery ? m_miceDiscovery->sinks()
                                                     : QVector<SinkDevice>{};
    QVector<bool> miceUsed(mice.size(), false);

    for (SinkDevice &peer : p2p) {
        const QString mac = peer.p2pMac.isEmpty() ? peer.address : peer.p2pMac;
        for (int i = 0; i < mice.size(); ++i) {
            if (miceUsed.at(i))
                continue;
            if (!MiceDiscovery::matchesP2p(mice.at(i), peer))
                continue;
            peer.miceHost = mice.at(i).miceHost;
            peer.miceCapable = true;
            if (peer.p2pMac.isEmpty())
                peer.p2pMac = mice.at(i).p2pMac;
            miceUsed[i] = true;
        }
        if (peer.miceHost.isEmpty() && !mac.isEmpty()) {
            const QString ip = MiceDiscovery::ipv4ForHardwareAddress(mac);
            if (!ip.isEmpty()) {
                peer.miceHost = ip;
                peer.miceCapable = true;
            }
        }
    }

    QVector<SinkDevice> next = p2p;
    for (int i = 0; i < mice.size(); ++i) {
        if (!miceUsed.at(i))
            next.append(mice.at(i));
    }
    if (m_dlnaDiscovery)
        next += m_dlnaDiscovery->renderers();
    m_sinks = next;
    Q_EMIT sinksChanged();
}

void CastEngine::updateScanStatus()
{
    if (m_state != SessionState::Scanning)
        return;

    int miracast = 0;
    int dlna = 0;
    for (const auto &sink : m_sinks) {
        if (sink.protocol == CastProtocol::Dlna)
            ++dlna;
        else
            ++miracast;
    }
    if (miracast == 0 && dlna == 0) {
        setStatusMessage(tr("Scanning for Miracast and DLNA displays…"));
        return;
    }
    setStatusMessage(tr("Found %1 Miracast, %2 DLNA.").arg(miracast).arg(dlna));
}

void CastEngine::finishScanIfReady()
{
    if (!m_p2pScanDone || !m_dlnaScanDone || !m_miceScanDone)
        return;
    if (m_state == SessionState::Connecting || m_state == SessionState::Streaming)
        return;

    mergeSinks();

    int miracast = 0;
    int dlna = 0;
    int wfd = 0;
    for (const auto &sink : m_sinks) {
        if (sink.protocol == CastProtocol::Dlna)
            ++dlna;
        else {
            ++miracast;
            if (sink.wfdCapable)
                ++wfd;
        }
    }

    if (m_sinks.isEmpty()) {
        setStatusMessage(tr("No displays found. Put the TV in Miracast mode, or keep it on this "
                            "Wi-Fi for DLNA."));
    } else if (dlna > 0 && miracast == 0) {
        setStatusMessage(tr("Scan finished. %1 DLNA renderer(s) available.").arg(dlna));
    } else if (dlna == 0 && wfd == 0) {
        setStatusMessage(tr("Found %1 P2P device(s) with no WFD IEs and no DLNA renderers. "
                            "wpa_supplicant may lack CONFIG_WIFI_DISPLAY.")
                             .arg(miracast));
    } else {
        setStatusMessage(tr("Scan finished. %1 Miracast, %2 DLNA.").arg(wfd).arg(dlna));
    }
    setState(SessionState::Idle);
}

void CastEngine::onPeersChanged()
{
    mergeSinks();
    updateScanStatus();
}

void CastEngine::onScanFinished()
{
    m_p2pScanDone = true;
    finishScanIfReady();
}

void CastEngine::onDlnaScanFinished()
{
    m_dlnaScanDone = true;
    finishScanIfReady();
}

void CastEngine::onMiceScanFinished()
{
    m_miceScanDone = true;
    finishScanIfReady();
}

void CastEngine::bindSession()
{
    m_p2p = std::make_unique<P2PSession>();
    m_dlna = std::make_unique<DlnaSession>();
    m_mice = std::make_unique<MiceSession>();
    m_wfd = std::make_unique<WfdServer>();
    m_encoder = std::make_unique<GstEncoder>();

    m_connectTimer.setSingleShot(true);
    connect(&m_connectTimer, &QTimer::timeout, this, [this]() {
        if (m_state != SessionState::Connecting)
            return;
        const SinkDevice sink = sinkById(m_selectedSinkId);
        if (sink.protocol == CastProtocol::Dlna) {
            failSession(tr("Timed out waiting for the TV to fetch the HTTP stream. "
                           "Allow inbound HTTP from the TV to this computer."));
        } else if (m_tryingMice) {
            fallbackToP2p(tr("Timed out waiting for WFD on the LAN. Allow inbound TCP 7236 "
                             "from the display, then retry."));
        } else {
            failSession(tr("Timed out forming the Wi-Fi Direct group or WFD session."));
        }
    });

    connect(m_p2p.get(), &P2PSession::statusChanged, this, &CastEngine::setStatusMessage);
    connect(m_p2p.get(), &P2PSession::activated, this, &CastEngine::onP2PActivated);
    connect(m_p2p.get(), &P2PSession::failed, this, &CastEngine::failSession);
    connect(m_p2p.get(), &P2PSession::deactivated, this, [this]() {
        if (!m_tearingDown
            && (m_state == SessionState::Streaming || m_state == SessionState::Connecting))
            failSession(tr("Wi-Fi Direct group dropped."));
    });

    connect(m_mice.get(), &MiceSession::statusChanged, this, &CastEngine::setStatusMessage);
    connect(m_mice.get(), &MiceSession::activated, this, &CastEngine::onMiceActivated);
    connect(m_mice.get(), &MiceSession::unavailable, this, [this](const QString &message) {
        if (m_state != SessionState::Connecting)
            return;
        fallbackToP2p(message);
    });
    connect(m_mice.get(), &MiceSession::failed, this, [this](const QString &message) {
        if (m_tearingDown)
            return;
        if (m_tryingMice && m_state == SessionState::Connecting) {
            fallbackToP2p(message);
            return;
        }
        failSession(message);
    });

    connect(m_dlna.get(), &DlnaSession::statusChanged, this, &CastEngine::setStatusMessage);
    connect(m_dlna.get(), &DlnaSession::failed, this, &CastEngine::failSession);

    connect(m_wfd.get(), &WfdServer::statusChanged, this, &CastEngine::setStatusMessage);
    connect(m_wfd.get(), &WfdServer::failed, this, &CastEngine::failSession);
    connect(m_wfd.get(), &WfdServer::playRequested, this, &CastEngine::onPlayRequested);

    connect(m_encoder.get(), &GstEncoder::started, this, [this]() {
        m_connectTimer.stop();
        m_tryingMice = false;
        setState(SessionState::Streaming);
        setStatusMessage(tr("Mirroring %1.").arg(m_encoder->streamDescription()));
        const SinkDevice sink = sinkById(m_selectedSinkId);
        logDeviceMatrix(sink, sink.protocol == CastProtocol::Dlna
                                  ? QStringLiteral("live-ts")
                                  : QStringLiteral("streaming"));
    });
    connect(m_encoder.get(), &GstEncoder::failed, this, &CastEngine::failSession);
}

void CastEngine::onP2PActivated(const QString &localIpv4)
{
    if (m_state != SessionState::Connecting)
        return;
    if (!m_wfd->listen(localIpv4, m_audioEnabled, m_sessionSource.width, m_sessionSource.height))
        return;
}

void CastEngine::onMiceActivated(const QString &localIpv4)
{
    if (m_state != SessionState::Connecting)
        return;
    if (!m_wfd->listen(localIpv4, m_audioEnabled, m_sessionSource.width, m_sessionSource.height))
        return;
    if (!m_mice->announce())
        fallbackToP2p(tr("Could not send SOURCE_READY on TCP 7250."));
}

void CastEngine::onPlayRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
                                const WfdAudioMode &audio)
{
    if (m_state != SessionState::Connecting && m_state != SessionState::Streaming)
        return;
    setStatusMessage(tr("Starting encoder (%1, %2, %3)…")
                         .arg(video.description(), audio.description(),
                              m_sessionSource.shortName()));
    m_encoder->start(sinkIp, rtpPort, video, audio, m_sessionSource);
}

void CastEngine::failSession(const QString &message)
{
    const SinkDevice sink = sinkById(m_selectedSinkId);
    QString result = QStringLiteral("failed");
    if (sink.protocol == CastProtocol::Dlna) {
        if (message.contains(QLatin1String("fetch the HTTP")))
            result = QStringLiteral("no-get");
        else if (sink.dlnaMedia == DlnaMediaKind::FileOnlyLikely
                 || message.contains(QLatin1String("file-only")))
            result = QStringLiteral("file-only");
        else if (message.contains(QLatin1String("SetAVTransportURI"))
                 || message.contains(QLatin1String("Play")))
            result = QStringLiteral("uri-reject");
    } else if (m_tryingMice) {
        result = QStringLiteral("mice-timeout");
    } else if (message.contains(QLatin1String("Timed out"))
               || message.contains(QLatin1String("group dropped"))) {
        result = QStringLiteral("p2p-timeout");
    }
    logDeviceMatrix(sink, result);

    teardownSession();
    setState(SessionState::Failed);
    setStatusMessage(message);
    Q_EMIT errorOccurred(message);
    setState(SessionState::Idle);
}

void CastEngine::logDeviceMatrix(const SinkDevice &sink, const QString &result) const
{
    const QString protocol = (sink.protocol == CastProtocol::Dlna)
        ? QStringLiteral("dlna")
        : QStringLiteral("miracast");
    const QString transport = (sink.protocol == CastProtocol::Dlna)
        ? QStringLiteral("http")
        : (sink.miceCapable || (m_mice && m_mice->active()) ? QStringLiteral("mice")
                                                            : QStringLiteral("p2p"));
    qInfo() << "device-matrix"
            << "protocol=" + protocol
            << "name=" + sink.name
            << "address=" + sink.address
            << "miceHost=" + sink.miceHost
            << "transport=" + transport
            << "hint=" + dlnaMediaKindKey(sink.dlnaMedia)
            << "summary=" + sink.dlnaMediaSummary
            << "result=" + result;
}

void CastEngine::teardownSession()
{
    if (m_tearingDown)
        return;
    m_tearingDown = true;
    cancelPairing();
    m_connectTimer.stop();
    m_tryingMice = false;
    if (m_dlna)
        m_dlna->stop();
    if (m_encoder)
        m_encoder->stop();
    if (m_wfd)
        m_wfd->stop();
    if (m_mice)
        m_mice->stop();
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
            const qreal dpr = screen->devicePixelRatio();
            const QRect g = scaleToNativePixels(screen->geometry(), dpr);
            source.x = g.x();
            source.y = g.y();
            source.width = g.width();
            source.height = g.height();
            qInfo() << "monitor" << source.id << "logical" << screen->geometry() << "dpr" << dpr
                    << "native" << g;
            source.primary = (screen == primary);
            source.name = tr("%1 (%2×%3)%4")
                              .arg(title)
                              .arg(source.width)
                              .arg(source.height)
                              .arg(source.primary ? tr(" · primary") : QString());
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
