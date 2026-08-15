#pragma once

#include "capture/displaysource.h"
#include "session/wfdaudiomode.h"
#include "session/wfdvideomode.h"

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

public Q_SLOTS:
    void start(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
               const WfdAudioMode &audio, const DisplaySource &source);
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
    bool startGst(const QString &sinkIp, quint16 rtpPort, bool withAudio);
    bool startFfmpeg(const QString &sinkIp, quint16 rtpPort, bool withAudio);

    QProcess m_process;
    QString m_lastError;
    WfdVideoMode m_video;
    WfdAudioMode m_audio;
    DisplaySource m_source;
    QString m_audioNote;
    bool m_audioActive = false;
    bool m_running = false;
};
