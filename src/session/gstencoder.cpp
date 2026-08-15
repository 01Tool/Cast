#include "session/gstencoder.h"

#include <QDebug>
#include <QStandardPaths>

GstEncoder::GstEncoder(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::errorOccurred, this, &GstEncoder::onProcessError);
    connect(&m_process, &QProcess::finished, this, &GstEncoder::onFinished);
}

GstEncoder::~GstEncoder()
{
    stop();
}

bool GstEncoder::running() const
{
    return m_running;
}

QString GstEncoder::lastError() const
{
    return m_lastError;
}

void GstEncoder::start(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &mode)
{
    stop();
    m_mode = mode.isValid() ? mode : defaultWfdVideoMode();

    if (sinkIp.isEmpty() || rtpPort == 0) {
        m_lastError = QStringLiteral("Missing sink IP or RTP port.");
        Q_EMIT failed(m_lastError);
        return;
    }

    // deepin plugins-bad 1.24.6 currently ships some 1.26 .so files
    // (mpegtsmux/h264parse) that GStreamer 1.24 refuses to load.
    if (gstHasElement(QStringLiteral("mpegtsmux"))
        && gstHasElement(QStringLiteral("h264parse"))
        && gstHasElement(QStringLiteral("ximagesrc"))
        && gstHasElement(QStringLiteral("x264enc"))) {
        if (startGst(sinkIp, rtpPort))
            return;
    } else {
        qWarning() << "GStreamer mpegtsmux/h264parse unavailable, using ffmpeg rtp_mpegts";
    }

    if (startFfmpeg(sinkIp, rtpPort))
        return;

    if (m_lastError.isEmpty()) {
        m_lastError = QStringLiteral(
            "No working encoder. Need a 1.24-compatible mpegtsmux or ffmpeg with libx264.");
    }
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

bool GstEncoder::startGst(const QString &sinkIp, quint16 rtpPort)
{
    const QString launch = QStandardPaths::findExecutable(QStringLiteral("gst-launch-1.0"));
    if (launch.isEmpty())
        return false;

    const QString pipeline = QStringLiteral(
                                 "ximagesrc use-damage=false show-pointer=true ! "
                                 "videoconvert ! videoscale ! "
                                 "video/x-raw,width=%1,height=%2,framerate=%3/1 ! "
                                 "x264enc tune=zerolatency speed-preset=ultrafast bitrate=4000 key-int-max=%3 ! "
                                 "video/x-h264,profile=baseline ! "
                                 "h264parse config-interval=1 ! "
                                 "mpegtsmux alignment=7 ! "
                                 "rtpmp2tpay pt=33 ! "
                                 "udpsink host=%4 port=%5 sync=false")
                                 .arg(m_mode.width)
                                 .arg(m_mode.height)
                                 .arg(m_mode.fps)
                                 .arg(sinkIp)
                                 .arg(rtpPort);
    qInfo() << "gst-launch" << pipeline;
    m_process.start(launch, QStringList{QStringLiteral("-e"), QStringLiteral("-q"), pipeline});
    if (!m_process.waitForStarted(3000)) {
        m_lastError = QStringLiteral("gst-launch-1.0 failed to start.");
        return false;
    }
    m_running = true;
    Q_EMIT started();
    return true;
}

bool GstEncoder::startFfmpeg(const QString &sinkIp, quint16 rtpPort)
{
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (ffmpeg.isEmpty()) {
        m_lastError = QStringLiteral("ffmpeg not found (needed because GStreamer mpegtsmux will not load).");
        return false;
    }

    const QString display = qEnvironmentVariable("DISPLAY", QStringLiteral(":0"));
    const QString url = QStringLiteral("rtp://%1:%2").arg(sinkIp).arg(rtpPort);
    const QStringList args{
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"),
        QStringLiteral("error"),
        QStringLiteral("-nostdin"),
        QStringLiteral("-f"),
        QStringLiteral("x11grab"),
        QStringLiteral("-framerate"),
        QString::number(m_mode.fps),
        QStringLiteral("-i"),
        display,
        QStringLiteral("-an"),
        QStringLiteral("-vf"),
        QStringLiteral("scale=%1:%2").arg(m_mode.width).arg(m_mode.height),
        QStringLiteral("-c:v"),
        QStringLiteral("libx264"),
        QStringLiteral("-preset"),
        QStringLiteral("ultrafast"),
        QStringLiteral("-tune"),
        QStringLiteral("zerolatency"),
        QStringLiteral("-profile:v"),
        QStringLiteral("baseline"),
        QStringLiteral("-g"),
        QString::number(m_mode.fps),
        QStringLiteral("-b:v"),
        QStringLiteral("4M"),
        QStringLiteral("-f"),
        QStringLiteral("rtp_mpegts"),
        url,
    };
    qInfo() << "ffmpeg" << args;
    m_process.start(ffmpeg, args);
    if (!m_process.waitForStarted(3000)) {
        m_lastError = QStringLiteral("ffmpeg failed to start.");
        return false;
    }
    m_running = true;
    Q_EMIT started();
    return true;
}

void GstEncoder::stop()
{
    m_running = false;
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
    Q_EMIT failed(m_lastError);
}

void GstEncoder::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_running)
        return;
    m_running = false;
    if (status != QProcess::NormalExit || exitCode != 0) {
        m_lastError = QString::fromLocal8Bit(m_process.readAllStandardError());
        if (m_lastError.isEmpty())
            m_lastError = QStringLiteral("encoder exited with code %1").arg(exitCode);
        Q_EMIT failed(m_lastError);
        return;
    }
    Q_EMIT stopped();
}
