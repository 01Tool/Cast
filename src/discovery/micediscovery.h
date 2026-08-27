#pragma once

#include "engine/sinkdevice.h"
#include "session/miceprotocol.h"

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QVector>

class MiceDiscovery : public QObject
{
    Q_OBJECT

public:
    static constexpr int DefaultScanSeconds = 8;

    explicit MiceDiscovery(QObject *parent = nullptr);
    ~MiceDiscovery() override;

    bool scanning() const;
    QVector<SinkDevice> sinks() const;

    static QString ipv4ForHardwareAddress(const QString &mac);
    static bool matchesP2p(const SinkDevice &mice, const SinkDevice &p2p);

public Q_SLOTS:
    void startScan(int timeoutSeconds = DefaultScanSeconds);
    void stopScan();

Q_SIGNALS:
    void sinksChanged();
    void scanFinished();
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onReadyRead();
    void onScanTimeout();
    void sendQuery();

private:
    bool bindSocket();
    void ingest(const MiceDnsService &svc, const QString &senderIpv4);

    QUdpSocket m_socket;
    QHash<QString, SinkDevice> m_sinks;
    QTimer m_scanTimer;
    QTimer m_repeatTimer;
    bool m_scanning = false;
};
