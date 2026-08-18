#include "session/gstencoder.h"

#include <QDebug>
#include <QStandardPaths>

GstEncoder::GstEncoder(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&m_process, &QProcess::errorOccurred, this, &GstEncoder::onProcessError);
    connect(&m_process, &QProcess::finished, this, &GstEncoder::onFinished);
}

QIODevice *GstEncoder::tsPipe()
{
    return &m_process;
}

GstEncoder::~GstEncoder()
{
    stop();
}

bool GstEncoder::running() const
{
    return m_running;
}

bool GstEncoder::audioActive() const
{
    return m_audioActive;
}

QString GstEncoder::lastError() const
{
    return m_lastError;
}

QString GstEncoder::streamDescription() const
{
    QString text = m_video.description();
    if (m_source.isValid())
        text += tr(" from %1").arg(m_source.shortName());
    if (m_audioActive)
        text += QStringLiteral(" + ") + m_audio.description();
    else if (!m_audioNote.isEmpty())
        text += QStringLiteral(" (") + m_audioNote + QLatin1Char(')');
    return text;
}

bool GstEncoder::prepare(const WfdVideoMode &video, const WfdAudioMode &audio,
                         const DisplaySource &source)
{
    stop();
    m_video = video.isValid() ? video : defaultWfdVideoMode();
    m_audio = audio;
    m_source = source;
    m_audioActive = false;
    m_audioNote.clear();
    m_lastError.clear();

    const bool wantAudio = m_audio.enabled();
    const QString monitor = wantAudio ? desktopPulseMonitor() : QString();
    if (wantAudio && monitor.isEmpty()) {
        m_audioNote = tr("no Pulse monitor, video only");
        qWarning() << m_audioNote;
    }
    return true;
}

bool GstEncoder::startPreferred(TsSink sink, const QString &sinkIp, quint16 rtpPort)
{
    const bool wantAudio = m_audio.enabled();
    const QString monitor = wantAudio ? desktopPulseMonitor() : QString();
    const bool gstVideo = gstHasElement(QStringLiteral("mpegtsmux"))
        && gstHasElement(QStringLiteral("h264parse"))
        && gstHasElement(QStringLiteral("ximagesrc"))
        && gstHasElement(QStringLiteral("x264enc"));
    const bool gstAudio = wantAudio && !monitor.isEmpty()
        && gstHasElement(QStringLiteral("pulsesrc"))
        && gstHasElement(QStringLiteral("audioconvert"))
        && gstHasElement(QStringLiteral("audioresample"))
        && gstHasElement(QStringLiteral("aacparse"))
        && !gstAacEncoder().isEmpty();

    if (gstVideo && (!wantAudio || gstAudio || monitor.isEmpty())) {
        if (startGst(sink, sinkIp, rtpPort, gstAudio))
            return true;
    } else if (!gstVideo) {
        qWarning() << "GStreamer mpegtsmux/h264parse unavailable, using ffmpeg";
    } else {
        qWarning() << "GStreamer AAC path unavailable, using ffmpeg for A/V";
    }

    if (startFfmpeg(sink, sinkIp, rtpPort, wantAudio && !monitor.isEmpty()))
        return true;

    if (m_lastError.isEmpty()) {
        m_lastError = tr("No working encoder. Need a 1.24-compatible mpegtsmux or ffmpeg with libx264.");
    }
    return false;
}

void GstEncoder::start(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
                       const WfdAudioMode &audio, const DisplaySource &source)
{
    prepare(video, audio, source);
    if (sinkIp.isEmpty() || rtpPort == 0) {
        m_lastError = tr("Missing sink IP or RTP port.");
        Q_EMIT failed(m_lastError);
        return;
    }
    if (startPreferred(TsSink::Rtp, sinkIp, rtpPort))
        return;
    Q_EMIT failed(m_lastError);
}

void GstEncoder::startMpegTsPipe(const WfdVideoMode &video, const WfdAudioMode &audio,
                                const DisplaySource &source)
{
    prepare(video, audio, source);
    if (startPreferred(TsSink::Stdout, {}, 0))
        return;
    Q_EMIT failed(m_lastError);
}

bool GstEncoder::gstHasElement(const QString &name) const
{
    const QString inspect = QStandardPaths::findExecutable(QStringLiteral("gst-inspect-1.0"));
    if (inspect.isEmpty())
        return false;
    QProcess probe;
    probe.start(inspect, QStringList{name});
    if (!probe.waitForFinished(2000)) {
        probe.kill();
        return false;
    }
    return probe.exitCode() == 0;
}

QString GstEncoder::gstAacEncoder() const
{
    if (gstHasElement(QStringLiteral("avenc_aac")))
        return QStringLiteral("avenc_aac compliance=experimental bitrate=128000");
    if (gstHasElement(QStringLiteral("voaacenc")))
        return QStringLiteral("voaacenc bitrate=128000");
    if (gstHasElement(QStringLiteral("faac")))
        return QStringLiteral("faac");
    return {};
}

QString GstEncoder::desktopPulseMonitor() const
{
    const QString env = qEnvironmentVariable("PULSE_SOURCE");
    if (!env.isEmpty())
        return env;

    const QString pactl = QStandardPaths::findExecutable(QStringLiteral("pactl"));
    if (pactl.isEmpty())
        return {};

    QProcess probe;
    probe.start(pactl, QStringList{QStringLiteral("get-default-sink")});
    if (!probe.waitForFinished(2000)) {
        probe.kill();
        return {};
    }
    if (probe.exitCode() != 0)
        return {};

    const QString sink = QString::fromLocal8Bit(probe.readAllStandardOutput()).trimmed();
    if (sink.isEmpty())
        return {};
    return sink + QStringLiteral(".monitor");
}

QString GstEncoder::ximagesrcElement() const
{
    QString element = QStringLiteral("ximagesrc use-damage=false show-pointer=true");
    const QString region = ximagesrcRegionProperties(m_source);
    if (!region.isEmpty())
        element += QLatin1Char(' ') + region;
    return element;
}

bool GstEncoder::startGst(TsSink sink, const QString &sinkIp, quint16 rtpPort, bool withAudio)
{
    const QString launch = QStandardPaths::findExecutable(QStringLiteral("gst-launch-1.0"));
    if (launch.isEmpty())
        return false;

    const QString monitor = withAudio ? desktopPulseMonitor() : QString();
    const QString grab = ximagesrcElement();
    const QString tsOut = (sink == TsSink::Stdout)
        ? QStringLiteral("fdsink fd=1 sync=false")
        : QStringLiteral("rtpmp2tpay pt=33 ! udpsink host=%1 port=%2 sync=false")
              .arg(sinkIp)
              .arg(rtpPort);
    const QString preset = x264Preset(sink);
    const QString profile = x264Profile(sink);
    const int bitrate = videoBitrateKbps();
    QString pipeline;
    if (withAudio && !monitor.isEmpty()) {
        const QString x264 = QStringLiteral(
                                 "x264enc tune=zerolatency speed-preset=%1 bitrate=%2 key-int-max=%3 ! "
                                 "video/x-h264,profile=%4")
                                 .arg(preset)
                                 .arg(bitrate)
                                 .arg(m_video.fps)
                                 .arg(profile);
        pipeline = QStringLiteral(
                       "mpegtsmux name=mux alignment=7 ! %1 "
                       "%2 ! "
                       "videoconvert ! videoscale ! "
                       "video/x-raw,width=%3,height=%4,framerate=%5/1 ! "
                       "%6 ! h264parse config-interval=1 ! queue ! mux. "
                       "pulsesrc device=\"%7\" provide-clock=true do-timestamp=true ! "
                       "audioconvert ! audioresample ! audio/x-raw,rate=%8,channels=2 ! "
                       "%9 ! aacparse ! queue ! mux.")
                       .arg(tsOut, grab)
                       .arg(m_video.width)
                       .arg(m_video.height)
                       .arg(m_video.fps)
                       .arg(x264, monitor)
                       .arg(m_audio.rate)
                       .arg(gstAacEncoder());
        m_audioActive = true;
    } else {
        pipeline = QStringLiteral(
                       "%1 ! "
                       "videoconvert ! videoscale ! "
                       "video/x-raw,width=%2,height=%3,framerate=%4/1 ! "
                       "x264enc tune=zerolatency speed-preset=%5 bitrate=%6 key-int-max=%4 ! "
                       "video/x-h264,profile=%7 ! "
                       "h264parse config-interval=1 ! "
                       "mpegtsmux alignment=7 ! %8")
                       .arg(grab)
                       .arg(m_video.width)
                       .arg(m_video.height)
                       .arg(m_video.fps)
                       .arg(preset)
                       .arg(bitrate)
                       .arg(profile, tsOut);
    }

    qInfo() << "gst-launch" << pipeline;
    m_process.start(launch, QStringList{QStringLiteral("-e"), QStringLiteral("-q"), pipeline});
    if (!m_process.waitForStarted(3000)) {
        m_lastError = tr("gst-launch-1.0 failed to start.");
        m_audioActive = false;
        return false;
    }
    m_running = true;
    Q_EMIT started();
    return true;
}

bool GstEncoder::startFfmpeg(TsSink sink, const QString &sinkIp, quint16 rtpPort, bool withAudio)
{
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (ffmpeg.isEmpty()) {
        m_lastError = tr("ffmpeg not found (needed because GStreamer mpegtsmux will not load).");
        return false;
    }

    const QString display = qEnvironmentVariable("DISPLAY", QStringLiteral(":0"));
    const QString monitor = withAudio ? desktopPulseMonitor() : QString();
    QStringList args{
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"),
        QStringLiteral("error"),
        QStringLiteral("-nostdin"),
        QStringLiteral("-f"),
        QStringLiteral("x11grab"),
        QStringLiteral("-framerate"),
        QString::number(m_video.fps),
    };
    const QString grabSize = x11grabSize(m_source);
    if (!grabSize.isEmpty())
        args << QStringLiteral("-video_size") << grabSize;
    args << QStringLiteral("-i") << x11grabInputSpecifier(display, m_source);
    if (withAudio && !monitor.isEmpty()) {
        args << QStringLiteral("-f") << QStringLiteral("pulse") << QStringLiteral("-i") << monitor;
        m_audioActive = true;
    } else if (withAudio) {
        m_audioNote = tr("no Pulse monitor, video only");
    }

    args << QStringLiteral("-vf")
         << QStringLiteral("scale=%1:%2:flags=lanczos,format=yuv420p")
                .arg(m_video.width)
                .arg(m_video.height)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
         << QStringLiteral("-c:v") << QStringLiteral("libx264")
         << QStringLiteral("-preset") << x264Preset(sink)
         << QStringLiteral("-tune") << QStringLiteral("zerolatency")
         << QStringLiteral("-profile:v") << x264Profile(sink)
         << QStringLiteral("-g") << QString::number(m_video.fps)
         << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(videoBitrateKbps());

    if (m_audioActive) {
        args << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-ar") << QString::number(m_audio.rate)
             << QStringLiteral("-ac") << QStringLiteral("2")
             << QStringLiteral("-b:a") << QStringLiteral("128k")
             << QStringLiteral("-af") << QStringLiteral("aresample=async=1:first_pts=0");
    } else {
        args << QStringLiteral("-an");
    }

    if (sink == TsSink::Stdout) {
        args << QStringLiteral("-flush_packets") << QStringLiteral("1")
             << QStringLiteral("-f") << QStringLiteral("mpegts")
             << QStringLiteral("pipe:1");
    } else {
        args << QStringLiteral("-f") << QStringLiteral("rtp_mpegts")
             << QStringLiteral("rtp://%1:%2").arg(sinkIp).arg(rtpPort);
    }

    qInfo() << "ffmpeg" << args;
    m_process.start(ffmpeg, args);
    if (!m_process.waitForStarted(3000)) {
        m_lastError = tr("ffmpeg failed to start.");
        m_audioActive = false;
        return false;
    }
    m_running = true;
    Q_EMIT started();
    return true;
}

void GstEncoder::stop()
{
    m_running = false;
    m_audioActive = false;
    if (m_process.state() == QProcess::NotRunning)
        return;
    m_process.terminate();
    if (!m_process.waitForFinished(2000))
        m_process.kill();
}

void GstEncoder::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    if (!m_running)
        return;
    m_lastError = m_process.errorString();
    m_running = false;
    m_audioActive = false;
    Q_EMIT failed(m_lastError);
}

void GstEncoder::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_running)
        return;
    m_running = false;
    m_audioActive = false;
    if (status != QProcess::NormalExit || exitCode != 0) {
        m_lastError = QString::fromLocal8Bit(m_process.readAllStandardError());
        if (m_lastError.isEmpty())
            m_lastError = tr("encoder exited with code %1").arg(exitCode);
        Q_EMIT failed(m_lastError);
        return;
    }
    Q_EMIT stopped();
}

int GstEncoder::videoBitrateKbps() const
{
    const int pixels = m_video.width * m_video.height;
    if (pixels >= 1920 * 1080)
        return 8000;
    if (pixels >= 1280 * 720)
        return 5000;
    return 3500;
}

QString GstEncoder::x264Preset(TsSink sink) const
{
    return (sink == TsSink::Stdout) ? QStringLiteral("veryfast") : QStringLiteral("ultrafast");
}

QString GstEncoder::x264Profile(TsSink sink) const
{
    return (sink == TsSink::Stdout) ? QStringLiteral("main") : QStringLiteral("baseline");
}
