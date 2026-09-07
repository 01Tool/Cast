#pragma once

#include "capture/displaysource.h"
#include "session/mediasource.h"
#include "session/wfdaudiomode.h"
#include "session/wfdvideomode.h"

#include <QIODevice>
#include <QObject>
#include <QProcess>
#include <QString>

class GstEncoder : public QObject
{
    Q_OBJECT

public:
    explicit GstEncoder(QObject *parent = nullptr);
    ~GstEncoder() override;

    bool running() const;
    bool audioActive() const;
    QString lastError() const;
    QString streamDescription() const;

    QIODevice *tsPipe();

public Q_SLOTS:
    void start(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
               const WfdAudioMode &audio, const DisplaySource &source,
               const MediaSource &media = MediaSource());
    void startMpegTsPipe(const WfdVideoMode &video, const WfdAudioMode &audio,
                         const DisplaySource &source, const MediaSource &media = MediaSource());
    void stop();

Q_SIGNALS:
    void started();
    void failed(const QString &message);
    void stopped();

private Q_SLOTS:
    void onProcessError(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    bool gstHasElement(const QString &name) const;
    QString gstAacEncoder() const;
    QString desktopPulseMonitor() const;
    QString ximagesrcElement() const;
    QString videoSourceElement() const;
    void attachPipeWireFd();
    enum class TsSink { Rtp, Stdout };

    bool prepare(const WfdVideoMode &video, const WfdAudioMode &audio, const DisplaySource &source,
                 const MediaSource &media);
    bool startPreferred(TsSink sink, const QString &sinkIp, quint16 rtpPort);
    bool startGst(TsSink sink, const QString &sinkIp, quint16 rtpPort, bool withAudio);
    bool startFfmpeg(TsSink sink, const QString &sinkIp, quint16 rtpPort, bool withAudio);
    void appendAudioEncodeArgs(QStringList *args) const;
    int videoBitrateKbps() const;
    QString x264Preset(TsSink sink) const;
    QString x264Profile(TsSink sink) const;

    QProcess m_process;
    QString m_lastError;
    WfdVideoMode m_video;
    WfdAudioMode m_audio;
    DisplaySource m_source;
    MediaSource m_media;
    QString m_audioNote;
    bool m_audioActive = false;
    bool m_running = false;
};
