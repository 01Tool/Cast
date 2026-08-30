#pragma once

#include "session/wfdaudiomode.h"
#include "session/wfdvideomode.h"

#include <QByteArray>
#include <QObject>
#include <QAbstractSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

class WfdSession : public QObject
{
    Q_OBJECT

public:
    explicit WfdSession(QTcpSocket *socket, const QString &localIpv4, bool audioWanted,
                       int sourceWidth, int sourceHeight, QObject *parent = nullptr);

Q_SIGNALS:
    void playRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
                       const WfdAudioMode &audio);
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
    WfdVideoMode m_video;
    WfdAudioMode m_audio;
    bool m_audioWanted = false;
    int m_sourceWidth = 0;
    int m_sourceHeight = 0;
    bool m_sentM1 = false;
    bool m_gotM2 = false;
};

class WfdServer : public QObject
{
    Q_OBJECT

public:
    explicit WfdServer(QObject *parent = nullptr);
    ~WfdServer() override;

    bool listen(const QString &localIpv4, bool audioWanted, int sourceWidth = 0,
                int sourceHeight = 0);
    // Some Android/MediaTek sinks become P2P GO and wait for the source to
    // open RTSP :7236 instead of dialing the source (WFD spec).
    void dial(const QString &peerIpv4);
    void stop();

Q_SIGNALS:
    void playRequested(const QString &sinkIp, quint16 rtpPort, const WfdVideoMode &video,
                       const WfdAudioMode &audio);
    void failed(const QString &message);
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onNewConnection();
    void onDialConnected();
    void onDialError(QAbstractSocket::SocketError error);
    void onDialTimeout();

private:
    void attachSession(QTcpSocket *socket);
    void stopDial();
    void scheduleRedial();

    QTcpServer m_server;
    QTcpSocket *m_dialSocket = nullptr;
    QTimer m_dialTimer;
    WfdSession *m_session = nullptr;
    QString m_localIpv4;
    QString m_peerIpv4;
    int m_dialTries = 0;
    bool m_audioWanted = false;
    int m_sourceWidth = 0;
    int m_sourceHeight = 0;
};
