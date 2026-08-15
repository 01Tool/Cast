#pragma once

#include "capture/displaysource.h"
#include "engine/sinkdevice.h"
#include "session/dlnaprofile.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QTcpServer>
#include <QUrl>

#include <functional>

class GstEncoder;
class QNetworkReply;
class QTcpSocket;

class DlnaSession : public QObject
{
    Q_OBJECT

public:
    explicit DlnaSession(QObject *parent = nullptr);
    ~DlnaSession() override;

    bool running() const;
    QUrl streamUrl() const;

public Q_SLOTS:
    void start(const SinkDevice &sink, const DisplaySource &source, bool audioEnabled,
               GstEncoder *encoder);
    void stop();

Q_SIGNALS:
    void statusChanged(const QString &message);
    void failed(const QString &message);
    void playIssued();

private:
    void fail(const QString &message);
    void beginControl();
    void queryProtocolInfo();
    void setUriAndPlay();
    void invoke(const QUrl &controlUrl, const QString &serviceType, const QString &action,
                const QString &innerXml, const std::function<void(bool, QByteArray)> &done);
    void onNewConnection();
    void handleClient(QTcpSocket *socket);
    void writeHeaders(QTcpSocket *socket, bool withBodyHint);
    void attachEncoder(QTcpSocket *socket);
    void detachClient();
    void soapStop();

    QTcpServer m_server;
    QNetworkAccessManager m_nam;
    SinkDevice m_sink;
    DisplaySource m_source;
    DlnaProfile m_profile;
    QUrl m_streamUrl;
    GstEncoder *m_encoder = nullptr;
    QTcpSocket *m_client = nullptr;
    bool m_running = false;
    bool m_stopping = false;
    bool m_audioEnabled = false;
};