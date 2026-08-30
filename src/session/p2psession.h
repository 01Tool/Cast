#pragma once

#include "engine/sinkdevice.h"

#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QTimer>

class QDBusPendingCallWatcher;

class P2PSession : public QObject
{
    Q_OBJECT

public:
    explicit P2PSession(QObject *parent = nullptr);
    ~P2PSession() override;

    bool active() const;
    QString localIpv4() const;
    QString peerIpv4() const;
    QString lastError() const;

public Q_SLOTS:
    void activate(const SinkDevice &sink);
    void deactivate();

Q_SIGNALS:
    void activated(const QString &localIpv4);
    void deactivated();
    void failed(const QString &message);
    void statusChanged(const QString &message);

private Q_SLOTS:
    void onAddFinished(QDBusPendingCallWatcher *watcher);
    void onActiveProperties(const QString &interface,
                            const QVariantMap &changed,
                            const QStringList &invalidated);
    void onActiveStateChanged(uint state, uint reason);
    void pollActiveState();

private:
    QVariant readProperty(const QString &path, const QString &interface, const QString &name) const;
    void watchActiveConnection(const QString &path);
    void unwatchActiveConnection();
    void handleActiveState(quint32 state);
    QString queryLocalIpv4() const;
    QString queryPeerIpv4() const;
    void finishActivated();

    QString m_activePath;
    QString m_localIpv4;
    QString m_peerIpv4;
    QString m_lastError;
    QTimer m_statePoll;
    bool m_active = false;
};
