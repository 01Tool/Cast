#pragma once

#include "engine/sinkdevice.h"

#include <QDBusObjectPath>
#include <QObject>
#include <QString>

class QDBusPendingCallWatcher;

class P2PSession : public QObject
{
    Q_OBJECT

public:
    explicit P2PSession(QObject *parent = nullptr);
    ~P2PSession() override;

    bool active() const;
    QString localIpv4() const;
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

private:
    QVariant readProperty(const QString &path, const QString &interface, const QString &name) const;
    void watchActiveConnection(const QString &path);
    void unwatchActiveConnection();
    QString queryLocalIpv4() const;
    void finishActivated();

    QString m_activePath;
    QString m_localIpv4;
    QString m_lastError;
    bool m_active = false;
};
