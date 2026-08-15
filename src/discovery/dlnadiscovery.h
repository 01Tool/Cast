#pragma once

#include "engine/sinkdevice.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>
#include <QVector>

class QNetworkReply;

class DlnaDiscovery : public QObject
{
    Q_OBJECT

public:
    static constexpr int DefaultScanSeconds = 20;

    explicit DlnaDiscovery(QObject *parent = nullptr);
    ~DlnaDiscovery() override;

    bool scanning() const;
    QVector<SinkDevice> renderers() const;

public Q_SLOTS:
    void startScan(int timeoutSeconds = DefaultScanSeconds);
    void stopScan();

Q_SIGNALS:
    void renderersChanged();
    void scanFinished();
    void errorOccurred(const QString &message);
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onReadyRead();
    void onScanTimeout();
    void sendMsearch();

private:
    void handleSsdp(const QByteArray &datagram, const QHostAddress &peer);
    void fetchDescription(const QUrl &location, const QHostAddress &peer);
    void ingestRenderer(const SinkDevice &sink);

    QUdpSocket m_socket;
    QNetworkAccessManager m_nam;
    QHash<QString, SinkDevice> m_renderers;
    QSet<QString> m_pendingLocations;
    QTimer m_scanTimer;
    QTimer m_repeatTimer;
    bool m_scanning = false;
};