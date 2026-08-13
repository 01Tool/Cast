#pragma once

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
    QString lastError() const;

public Q_SLOTS:
    void start(const QString &sinkIp, quint16 rtpPort);
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
    bool startGst(const QString &sinkIp, quint16 rtpPort);
    bool startFfmpeg(const QString &sinkIp, quint16 rtpPort);

    QProcess m_process;
    QString m_lastError;
    bool m_running = false;
};
