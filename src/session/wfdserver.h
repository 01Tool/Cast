#pragma once

#include "session/wfdvideomode.h"

#include <QByteArray>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class WfdSession : public QObject
{
    Q_OBJECT

public:
    explicit WfdSession(QTcpSocket *socket, const QString &localIpv4, QObject *parent = nullptr);

Q_SIGNALS:
    void playRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &mode);
    void sessionClosed();
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onReadyRead();
    void onDisconnected();
    void sendSourceOptions();

private:
    void handleMessage(const QByteArray &raw);
    void handleRequest(const QString &method, int cseq, const QByteArray &body,
                       const QString &transport);
    void handleResponse(int cseq, const QByteArray &body);
    void sendResponse(int cseq, const QByteArray &extraHeaders = {}, const QByteArray &body = {});
    void sendRequest(const QByteArray &method, const QByteArray &body = {});
    void sendGetParameter();
    void sendSetParameter();
    void sendTriggerSetup();
    void parseSinkParams(const QByteArray &body);

    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    QString m_localIpv4;
    int m_nextCseq = 1;
    int m_pendingGetCseq = -1;
    int m_pendingSetCseq = -1;
    int m_pendingTriggerCseq = -1;
    quint16 m_rtpPort = 0;
    WfdVideoMode m_mode;
    bool m_sentM1 = false;
    bool m_gotM2 = false;
};

class WfdServer : public QObject
{
    Q_OBJECT

public:
    explicit WfdServer(QObject *parent = nullptr);
    ~WfdServer() override;

    bool listen(const QString &localIpv4);
    void stop();

Q_SIGNALS:
    void playRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &mode);
    void failed(const QString &message);
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onNewConnection();

private:
    QTcpServer m_server;
    WfdSession *m_session = nullptr;
    QString m_localIpv4;
};
