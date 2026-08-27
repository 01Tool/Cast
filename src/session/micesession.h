#pragma once

#include "engine/sinkdevice.h"

#include <QByteArray>
#include <QHostInfo>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

class MiceSession : public QObject
{
    Q_OBJECT

public:
    explicit MiceSession(QObject *parent = nullptr);
    ~MiceSession() override;

    bool active() const;
    QString localIpv4() const;
    QString remoteIpv4() const;

public Q_SLOTS:
    void start(const SinkDevice &sink);
    bool announce();
    void stop();

Q_SIGNALS:
    void activated(const QString &localIpv4);
    void unavailable(const QString &message);
    void failed(const QString &message);
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onLookedUp(const QHostInfo &info);
    void onConnected();
    void onError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onConnectTimeout();

private:
    void resolveNext();
    void connectHost(const QString &host);
    void sendStop();
    void emitUnavailable(const QString &message);

    SinkDevice m_sink;
    QTcpSocket m_socket;
    QTimer m_connectTimer;
    QStringList m_lookupNames;
    int m_lookupIndex = 0;
    quint32 m_lookupId = 0;
    QString m_host;
    QString m_localIpv4;
    QString m_friendlyName;
    QByteArray m_sourceId;
    QByteArray m_rx;
    bool m_active = false;
    bool m_announced = false;
    bool m_stopping = false;
    bool m_reported = false;
};
