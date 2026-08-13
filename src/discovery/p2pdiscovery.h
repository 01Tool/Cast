#pragma once

#include "engine/sinkdevice.h"

#include <QDBusObjectPath>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

class P2PDiscovery : public QObject
{
    Q_OBJECT

public:
    static constexpr int DefaultScanSeconds = 20;

    explicit P2PDiscovery(QObject *parent = nullptr);
    ~P2PDiscovery() override;

    bool scanning() const;
    QVector<SinkDevice> peers() const;

public Q_SLOTS:
    void startScan(int timeoutSeconds = DefaultScanSeconds);
    void stopScan();

Q_SIGNALS:
    void peersChanged();
    void scanFinished();
    void errorOccurred(const QString &message);
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onPeerAdded(const QDBusObjectPath &path);
    void onPeerRemoved(const QDBusObjectPath &path);
    void onScanTimeout();

private:
    QStringList findP2PDevicePaths() const;
    bool wifiEnabled() const;
    QVariant readProperty(const QString &service,
                          const QString &path,
                          const QString &interface,
                          const QString &name) const;
    SinkDevice readPeer(const QString &path) const;
    void upsertPeer(const QString &path);
    void removePeer(const QString &path);
    void connectDeviceSignals(const QString &devicePath);
    void disconnectDeviceSignals();
    void stopFindOnDevices();
    void tryAdvertiseWfdIes();
    void loadExistingPeers(const QString &devicePath);

    QStringList m_devicePaths;
    QHash<QString, SinkDevice> m_peers;
    QTimer m_scanTimer;
    bool m_scanning = false;
};
