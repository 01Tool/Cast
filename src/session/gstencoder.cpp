#include "session/gstencoder.h"

#include "capture/screencastportal.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <cstdio>
#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <unistd.h>

namespace {

void dieIfParentDies()
{
    ::prctl(PR_SET_PDEATHSIG, SIGKILL);
    if (::getppid() == 1)
        ::_exit(127);
}

} // namespace

GstEncoder::GstEncoder(QObject *parent)
    : QObject(parent)
{
    auto bindProcess = [this](QProcess *proc, const char *tag) {
        proc->setProcessChannelMode(QProcess::SeparateChannels);
        connect(proc, &QProcess::errorOccurred, this, &GstEncoder::onProcessError);
        connect(proc, &QProcess::finished, this, &GstEncoder::onFinished);
        connect(proc, &QProcess::readyReadStandardError, this, [this, proc, tag]() {
            const QByteArray err = proc->readAllStandardError().trimmed();
            if (!err.isEmpty())
                qWarning() << tag << err;
        });
    };
    bindProcess(&m_process, "encoder");
    bindProcess(&m_videoProcess, "video encoder");
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
    if (m_media.isFile())
        text += tr(" from %1").arg(m_media.title);
    else if (m_source.isValid())
        text += tr(" from %1").arg(m_source.shortName());
    if (m_audioActive)
        text += QStringLiteral(" + ") + m_audio.description();
    else if (!m_audioNote.isEmpty())
        text += QStringLiteral(" (") + m_audioNote + QLatin1Char(')');
    return text;
}

bool GstEncoder::prepare(const WfdVideoMode &video, const WfdAudioMode &audio,
                         const DisplaySource &source, const MediaSource &media)
{
    stop();
    m_video = video.isValid() ? video : defaultWfdVideoMode();
    m_audio = audio;
    m_source = source;
    m_media = media;
    m_audioActive = false;
    m_audioNote.clear();
    m_lastError.clear();

    if (m_media.isFile())
        return true;
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
    if (m_media.isFile()) {
        const bool fileAudio = m_media.kind != MediaKind::Image
            && (m_media.kind == MediaKind::Audio || m_audio.enabled());
        if (startFfmpeg(sink, sinkIp, rtpPort, fileAudio))
            return true;
        if (m_lastError.isEmpty())
            m_lastError = tr("Could not encode the selected file.");
        return false;
    }

    const bool pipewire = m_source.pipewireFd >= 0;
    if (pipewire) {
        const bool gstPw = gstHasElement(QStringLiteral("mpegtsmux"))
            && gstHasElement(QStringLiteral("h264parse"))
            && gstHasElement(QStringLiteral("pipewiresrc"))
            && gstHasElement(QStringLiteral("x264enc"));
        if (!gstPw) {
            m_lastError = tr("Wayland capture needs GStreamer pipewiresrc "
                             "(package gstreamer1.0-pipewire) and mpegtsmux.");
            return false;
        }
        const bool wantAudio = m_audio.enabled();
        const QString monitor = wantAudio ? desktopPulseMonitor() : QString();
        const bool gstAac = wantAudio && m_audio.codec == WfdAudioMode::Codec::Aac && !monitor.isEmpty()
            && gstHasElement(QStringLiteral("pulsesrc"))
            && gstHasElement(QStringLiteral("audioconvert"))
            && gstHasElement(QStringLiteral("audioresample"))
            && gstHasElement(QStringLiteral("aacparse"))
            && !gstAacEncoder().isEmpty();
        const bool gstLpcm = wantAudio && m_audio.codec == WfdAudioMode::Codec::Lpcm
            && !monitor.isEmpty()
            && !QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty()
            && !QStandardPaths::findExecutable(QStringLiteral("gst-launch-1.0")).isEmpty();
        if (wantAudio && !gstAac && !gstLpcm) {
            m_audioNote = monitor.isEmpty() ? m_audioNote
                                            : tr("no LPCM mux on this path, video only");
            qWarning() << m_audioNote;
        }
        if (gstLpcm) {
            qInfo() << "PipeWire LPCM mux via gst-launch | ffmpeg";
            return startPipewireLpcm(sink, sinkIp, rtpPort);
        }
        return startGst(sink, sinkIp, rtpPort, gstAac);
    }

    const bool wantAudio = m_audio.enabled();
    const QString monitor = wantAudio ? desktopPulseMonitor() : QString();
    const bool gstVideo = gstHasElement(QStringLiteral("mpegtsmux"))
        && gstHasElement(QStringLiteral("h264parse"))
        && gstHasElement(QStringLiteral("ximagesrc"))
        && gstHasElement(QStringLiteral("x264enc"));
    const bool gstAac = wantAudio && m_audio.codec == WfdAudioMode::Codec::Aac && !monitor.isEmpty()
        && gstHasElement(QStringLiteral("pulsesrc"))
        && gstHasElement(QStringLiteral("audioconvert"))
        && gstHasElement(QStringLiteral("audioresample"))
        && gstHasElement(QStringLiteral("aacparse"))
        && !gstAacEncoder().isEmpty();

    if (gstVideo && (!wantAudio || gstAac || monitor.isEmpty())) {
        if (startGst(sink, sinkIp, rtpPort, gstAac))
            return true;
    } else if (!gstVideo) {
        qWarning() << "GStreamer mpegtsmux/h264parse unavailable, using ffmpeg";
    } else {
        qWarning() << "GStreamer audio path unavailable, using ffmpeg for A/V";
    }

    if (startFfmpeg(sink, sinkIp, rtpPort, wantAudio && !monitor.isEmpty()))
        return true;

    if (m_lastError.isEmpty()) {
        m_lastError = tr("No working encoder. Need a 1.24-compatible mpegtsmux or ffmpeg with libx264.");
    }
    return false;
}

void GstEncoder::start(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
                       const WfdAudioMode &audio, const DisplaySource &source,
                       const MediaSource &media)
{
    prepare(video, audio, source, media);
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
                                const DisplaySource &source, const MediaSource &media)
{
    if (m_running)
        return;
    prepare(video, audio, source, media);
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

QString GstEncoder::videoSourceElement() const
{
    if (m_source.pipewireFd >= 0) {
        // Portal remotes already target one node. autoconnect=true races a
        // second link and fails with "no more output formats". always-copy
        // turns Treeland DMA-BUF into CPU video/x-raw for videoconvert/x264enc.
        // Do not set path/autoconnect=false: a stale node id waits forever
        // (0% CPU, no MPEG-TS). The portal remote only exposes this stream.
        return QStringLiteral(
            "pipewiresrc fd=%1 always-copy=true provide-clock=false "
            "do-timestamp=true keepalive-time=1000")
            .arg(ScreenCastPortal::gstPipeWireFd);
    }
    return ximagesrcElement();
}

QString GstEncoder::pipewireH264Pipeline(TsSink sink) const
{
    return QStringLiteral(
               "%1 ! queue max-size-buffers=8 leaky=downstream ! "
               "videoconvert ! videoscale add-borders=true method=4 ! "
               "video/x-raw,width=%2,height=%3 ! "
               "x264enc tune=zerolatency speed-preset=%4 bitrate=%5 key-int-max=%6 "
               "bframes=0 byte-stream=true ! "
               "video/x-h264,stream-format=byte-stream,alignment=au,profile=%7 ! "
               "h264parse config-interval=1")
        .arg(videoSourceElement())
        .arg(m_video.width)
        .arg(m_video.height)
        .arg(x264Preset(sink))
        .arg(videoBitrateKbps())
        .arg(m_video.fps)
        .arg(x264Profile(sink));
}

QString GstEncoder::pipewireRawI420Pipeline() const
{
    // I420 for ffmpeg -f rawvideo -pixel_format yuv420p. Do not set framerate
    // here: Treeland pipewiresrc often has framerate=0/1 and videorate stalls.
    return QStringLiteral(
               "%1 ! queue max-size-buffers=2 leaky=downstream ! "
               "videoconvert ! videoscale add-borders=true method=4 ! "
               "video/x-raw,format=I420,width=%2,height=%3 ! "
               "queue max-size-buffers=2 leaky=downstream")
        .arg(videoSourceElement())
        .arg(m_video.width)
        .arg(m_video.height);
}

void GstEncoder::attachPipeWireFd(QProcess *proc)
{
    if (!proc)
        proc = &m_process;
    proc->setChildProcessModifier({});
    const int srcFd = m_source.pipewireFd;
    if (srcFd < 0)
        return;
    qInfo() << "attach PipeWire parent fd" << srcFd << "-> child"
            << ScreenCastPortal::gstPipeWireFd;
    std::fprintf(stderr, "ot-cast: attach PipeWire parent fd %d -> child %d\n", srcFd,
                 ScreenCastPortal::gstPipeWireFd);
    std::fflush(stderr);
    ::fcntl(srcFd, F_SETFD, 0);
    proc->setChildProcessModifier([srcFd]() {
        dieIfParentDies();
        if (srcFd != ScreenCastPortal::gstPipeWireFd) {
            if (::dup2(srcFd, ScreenCastPortal::gstPipeWireFd) == -1)
                ::_exit(127);
        }
        ::fcntl(ScreenCastPortal::gstPipeWireFd, F_SETFD, 0);
    });
}

void GstEncoder::stopProcess(QProcess *proc)
{
    if (!proc || proc->state() == QProcess::NotRunning)
        return;
    proc->setChildProcessModifier({});
    proc->terminate();
    if (!proc->waitForFinished(2000))
        proc->kill();
}

void GstEncoder::closeH264Pipe()
{
    if (m_h264ReadFd >= 0) {
        ::close(m_h264ReadFd);
        m_h264ReadFd = -1;
    }
    if (m_h264WriteFd >= 0) {
        ::close(m_h264WriteFd);
        m_h264WriteFd = -1;
    }
}

void GstEncoder::killOrphanRtpEncoders() const
{
    const qint64 self = QCoreApplication::applicationPid();
    const qint64 child = m_process.processId();
    const qint64 child2 = m_videoProcess.processId();
    const QDir proc(QStringLiteral("/proc"));
    for (const QString &name : proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool ok = false;
        const qint64 pid = name.toLongLong(&ok);
        if (!ok || pid <= 1 || pid == self || pid == child || pid == child2)
            continue;
        const QString exe = QFile::symLinkTarget(QStringLiteral("/proc/%1/exe").arg(pid));
        if (!exe.contains(QLatin1String("ffmpeg")))
            continue;
        QFile cmdlineFile(QStringLiteral("/proc/%1/cmdline").arg(pid));
        if (!cmdlineFile.open(QIODevice::ReadOnly))
            continue;
        const QByteArray cmd = cmdlineFile.readAll();
        if (!cmd.contains("rtp_mpegts") || !cmd.contains("pcm_bluray"))
            continue;
        qWarning() << "killing leftover LPCM ffmpeg" << pid << exe;
        ::kill(static_cast<pid_t>(pid), SIGTERM);
    }
}

bool GstEncoder::startGst(TsSink sink, const QString &sinkIp, quint16 rtpPort, bool withAudio)
{
    const QString launch = QStandardPaths::findExecutable(QStringLiteral("gst-launch-1.0"));
    if (launch.isEmpty())
        return false;

    const QString monitor = withAudio ? desktopPulseMonitor() : QString();
    const QString tsOut = (sink == TsSink::Stdout)
        ? QStringLiteral("fdsink fd=1 sync=false")
        : QStringLiteral("rtpmp2tpay pt=33 ! udpsink host=%1 port=%2 sync=false")
              .arg(sinkIp)
              .arg(rtpPort);
    QString pipeline;
    if (withAudio && !monitor.isEmpty()) {
        const QString x264 = QStringLiteral(
                                 "x264enc tune=zerolatency speed-preset=%1 bitrate=%2 key-int-max=%3 "
                                 "bframes=0 byte-stream=true ! "
                                 "video/x-h264,stream-format=byte-stream,alignment=au,profile=%4")
                                 .arg(x264Preset(sink))
                                 .arg(videoBitrateKbps())
                                 .arg(m_video.fps)
                                 .arg(x264Profile(sink));
        // alignment=0 flushes every packet (HTTP, not RTP). pulsesrc must not
        // provide the clock or mpegtsmux waits forever on a second live source.
        pipeline = QStringLiteral(
                       "mpegtsmux name=mux alignment=0 ! %1 "
                       "%2 ! queue max-size-buffers=8 leaky=downstream ! "
                       "videoconvert ! videoscale add-borders=true method=4 ! "
                       "video/x-raw,width=%3,height=%4 ! "
                       "%5 ! h264parse config-interval=1 ! queue ! mux. "
                       "pulsesrc device=%6 provide-clock=false do-timestamp=true ! "
                       "queue max-size-time=3000000000 max-size-buffers=0 max-size-bytes=0 "
                       "leaky=downstream ! "
                       "audioconvert ! audioresample ! audio/x-raw,rate=%7,channels=2 ! "
                       "%8 ! aacparse ! queue ! mux.")
                       .arg(tsOut, videoSourceElement())
                       .arg(m_video.width)
                       .arg(m_video.height)
                       .arg(x264, monitor)
                       .arg(m_audio.rate)
                       .arg(gstAacEncoder());
        m_audioActive = true;
    } else {
        pipeline = pipewireH264Pipeline(sink) + QStringLiteral(" ! mpegtsmux alignment=0 ! ")
            + tsOut;
    }

    qInfo() << "gst-launch" << pipeline;
    std::fprintf(stderr, "ot-cast: gst-launch %s\n", qPrintable(pipeline));
    std::fflush(stderr);
    attachPipeWireFd();
    // gst-launch treats each argv token as a pipeline word. One string
    // containing spaces is a single invalid element name ("syntax error").
    QStringList args{QStringLiteral("-e"), QStringLiteral("-q")};
    args += pipeline.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    m_process.start(launch, args);
    if (!m_process.waitForStarted(3000)) {
        m_lastError = tr("gst-launch-1.0 failed to start.");
        m_audioActive = false;
        return false;
    }
    m_running = true;
    Q_EMIT started();
    return true;
}

bool GstEncoder::startPipewireLpcm(TsSink sink, const QString &sinkIp, quint16 rtpPort)
{
    const QString launch = QStandardPaths::findExecutable(QStringLiteral("gst-launch-1.0"));
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    const QString monitor = desktopPulseMonitor();
    if (launch.isEmpty() || ffmpeg.isEmpty() || monitor.isEmpty()) {
        m_lastError = tr("PipeWire LPCM needs gst-launch-1.0, ffmpeg, and a Pulse monitor.");
        return false;
    }

    killOrphanRtpEncoders();

    // Xiaomi plays X11 ffmpeg libx264+pcm_bluray. Copying gst H.264 into ffmpeg
    // failed the Pad: probe "unspecified size", then pcm_bluray as private
    // 0x06 with silent audio. Feed I420 on fd 4 and let ffmpeg encode both,
    // same as x11grab+pulse. rawvideo needs no SPS probe.
    closeH264Pipe();
    int fds[2] = {-1, -1};
    if (::pipe(fds) != 0) {
        m_lastError = tr("Could not create the H.264 pipe for LPCM mux.");
        return false;
    }
    m_h264ReadFd = fds[0];
    m_h264WriteFd = fds[1];
    ::fcntl(m_h264ReadFd, F_SETFD, FD_CLOEXEC);
    ::fcntl(m_h264WriteFd, F_SETFD, FD_CLOEXEC);

    const QString pipeline = pipewireRawI420Pipeline()
        + QStringLiteral(" ! fdsink fd=%1 sync=false").arg(kH264PipeFd);
    QStringList gstArgs{QStringLiteral("-e"), QStringLiteral("-q")};
    gstArgs += pipeline.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    const int pwFd = m_source.pipewireFd;
    const int h264Write = m_h264WriteFd;
    const int h264Read = m_h264ReadFd;
    const int pwChild = ScreenCastPortal::gstPipeWireFd;
    if (pwFd >= 0)
        ::fcntl(pwFd, F_SETFD, 0);
    m_videoProcess.setChildProcessModifier([pwFd, h264Write, pwChild]() {
        dieIfParentDies();
        if (pwFd >= 0) {
            if (pwFd != pwChild && ::dup2(pwFd, pwChild) == -1)
                ::_exit(127);
            ::fcntl(pwChild, F_SETFD, 0);
        }
        if (::dup2(h264Write, GstEncoder::kH264PipeFd) == -1)
            ::_exit(127);
        ::fcntl(GstEncoder::kH264PipeFd, F_SETFD, 0);
        if (h264Write != GstEncoder::kH264PipeFd)
            ::close(h264Write);
    });
    m_process.setChildProcessModifier([h264Read]() {
        dieIfParentDies();
        if (::dup2(h264Read, GstEncoder::kH264PipeFd) == -1)
            ::_exit(127);
        ::fcntl(GstEncoder::kH264PipeFd, F_SETFD, 0);
        if (h264Read != GstEncoder::kH264PipeFd)
            ::close(h264Read);
    });

    QStringList ffArgs{
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"),
        QStringLiteral("warning"),
        QStringLiteral("-nostdin"),
        QStringLiteral("-thread_queue_size"),
        QStringLiteral("4096"),
        QStringLiteral("-f"),
        QStringLiteral("pulse"),
        QStringLiteral("-i"),
        monitor,
        QStringLiteral("-thread_queue_size"),
        QStringLiteral("4096"),
        QStringLiteral("-f"),
        QStringLiteral("rawvideo"),
        QStringLiteral("-pixel_format"),
        QStringLiteral("yuv420p"),
        QStringLiteral("-video_size"),
        QStringLiteral("%1x%2").arg(m_video.width).arg(m_video.height),
        QStringLiteral("-framerate"),
        QString::number(m_video.fps),
        QStringLiteral("-i"),
        QStringLiteral("pipe:%1").arg(kH264PipeFd),
        QStringLiteral("-map"),
        QStringLiteral("1:v:0"),
        QStringLiteral("-map"),
        QStringLiteral("0:a:0"),
        QStringLiteral("-pix_fmt"),
        QStringLiteral("yuv420p"),
        QStringLiteral("-c:v"),
        QStringLiteral("libx264"),
        QStringLiteral("-preset"),
        x264Preset(sink),
        QStringLiteral("-tune"),
        QStringLiteral("zerolatency"),
        QStringLiteral("-profile:v"),
        x264Profile(sink),
        QStringLiteral("-g"),
        QString::number(m_video.fps),
        QStringLiteral("-b:v"),
        QStringLiteral("%1k").arg(videoBitrateKbps()),
        QStringLiteral("-mpegts_muxer_options"),
        QStringLiteral("mpegts_flags=+resend_headers+pat_pmt_at_frames"),
    };
    appendAudioEncodeArgs(&ffArgs, true);
    if (sink == TsSink::Stdout) {
        ffArgs << QStringLiteral("-flush_packets") << QStringLiteral("1")
               << QStringLiteral("-f") << QStringLiteral("mpegts")
               << QStringLiteral("pipe:1");
    } else {
        ffArgs << QStringLiteral("-f") << QStringLiteral("rtp_mpegts")
               << QStringLiteral("rtp://%1:%2").arg(sinkIp).arg(rtpPort);
    }

    qInfo() << "gst-launch I420 | ffmpeg LPCM" << pipeline << ffArgs;
    std::fprintf(stderr, "ot-cast: gst-launch I420 | ffmpeg LPCM fd=%d %s\n", kH264PipeFd,
                 qPrintable(pipeline));
    std::fflush(stderr);

    m_process.start(ffmpeg, ffArgs);
    if (!m_process.waitForStarted(3000)) {
        m_lastError = tr("ffmpeg failed to start.");
        closeH264Pipe();
        return false;
    }
    m_videoProcess.start(launch, gstArgs);
    if (!m_videoProcess.waitForStarted(3000)) {
        m_lastError = tr("gst-launch-1.0 failed to start.");
        stopProcess(&m_process);
        closeH264Pipe();
        return false;
    }
    // Children hold fd 4. Drop the parent copies so ffmpeg sees EOF when gst
    // exits instead of hanging on the parent write end.
    closeH264Pipe();
    m_audioActive = true;
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

    QStringList args{
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"),
        QStringLiteral("error"),
        QStringLiteral("-nostdin"),
    };

    if (m_media.isFile()) {
        if (m_media.kind == MediaKind::Image) {
            args << QStringLiteral("-loop") << QStringLiteral("1")
                 << QStringLiteral("-framerate") << QString::number(m_video.fps)
                 << QStringLiteral("-i") << m_media.path;
            m_audioActive = false;
        } else if (m_media.kind == MediaKind::Audio) {
            args << QStringLiteral("-re") << QStringLiteral("-i") << m_media.path
                 << QStringLiteral("-f") << QStringLiteral("lavfi") << QStringLiteral("-i")
                 << QStringLiteral("color=c=black:s=%1x%2:r=%3")
                        .arg(m_video.width)
                        .arg(m_video.height)
                        .arg(m_video.fps)
                 << QStringLiteral("-map") << QStringLiteral("1:v:0")
                 << QStringLiteral("-map") << QStringLiteral("0:a:0");
            m_audioActive = true;
        } else {
            args << QStringLiteral("-re") << QStringLiteral("-i") << m_media.path
                 << QStringLiteral("-map") << QStringLiteral("0:v:0");
            if (withAudio)
                args << QStringLiteral("-map") << QStringLiteral("0:a:0?");
            m_audioActive = withAudio;
        }
    } else if (m_source.pipewireFd >= 0) {
        m_lastError = tr("ffmpeg cannot consume a PipeWire ScreenCast fd. Need gst-launch pipewiresrc.");
        return false;
    } else {
        const QString display = qEnvironmentVariable("DISPLAY", QStringLiteral(":0"));
        const QString monitor = withAudio ? desktopPulseMonitor() : QString();
        args << QStringLiteral("-f") << QStringLiteral("x11grab")
             << QStringLiteral("-framerate") << QString::number(m_video.fps);
        const QString grabSize = x11grabSize(m_source);
        if (!grabSize.isEmpty())
            args << QStringLiteral("-video_size") << grabSize;
        args << QStringLiteral("-i") << x11grabInputSpecifier(display, m_source);
        if (withAudio && !monitor.isEmpty()) {
            args << QStringLiteral("-f") << QStringLiteral("pulse") << QStringLiteral("-i")
                 << monitor;
            m_audioActive = true;
        } else if (withAudio) {
            m_audioNote = tr("no Pulse monitor, video only");
        }
    }

    args << QStringLiteral("-vf")
         << QStringLiteral("scale=%1:%2:force_original_aspect_ratio=decrease:flags=lanczos,"
                           "pad=%1:%2:(ow-iw)/2:(oh-ih)/2,format=yuv420p")
                .arg(m_video.width)
                .arg(m_video.height)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
         << QStringLiteral("-c:v") << QStringLiteral("libx264")
         << QStringLiteral("-preset") << x264Preset(sink)
         << QStringLiteral("-tune") << QStringLiteral("zerolatency")
         << QStringLiteral("-profile:v") << x264Profile(sink)
         << QStringLiteral("-g") << QString::number(m_video.fps)
         << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(videoBitrateKbps());

    if (m_audioActive)
        appendAudioEncodeArgs(&args);
    else
        args << QStringLiteral("-an");

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
    stopProcess(&m_process);
    stopProcess(&m_videoProcess);
    closeH264Pipe();
}

void GstEncoder::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    if (!m_running)
        return;
    auto *proc = qobject_cast<QProcess *>(sender());
    m_lastError = proc ? proc->errorString() : tr("encoder failed");
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

void GstEncoder::appendAudioEncodeArgs(QStringList *args, bool zeroFirstPts) const
{
    if (!args)
        return;
    *args << QStringLiteral("-ar") << QString::number(m_audio.rate)
          << QStringLiteral("-ac") << QStringLiteral("2")
          << QStringLiteral("-af")
          << (zeroFirstPts ? QStringLiteral("aresample=async=1:first_pts=0")
                           : QStringLiteral("aresample=async=1"));
    if (m_audio.codec == WfdAudioMode::Codec::Lpcm) {
        // 48 kHz WFD LPCM is HDMV/Blu-ray PCM in MPEG-TS. pcm_bluray does not
        // accept 44.1 kHz, so that rate uses raw big-endian PCM.
        *args << QStringLiteral("-c:a")
              << (m_audio.rate == 44100 ? QStringLiteral("pcm_s16be")
                                        : QStringLiteral("pcm_bluray"));
    } else {
        *args << QStringLiteral("-c:a") << QStringLiteral("aac")
              << QStringLiteral("-b:a") << QStringLiteral("128k");
    }
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
