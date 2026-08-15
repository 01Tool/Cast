#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QString>

// Registers as org.freedesktop.NetworkManager.SecretAgent so WPS PIN / PBC
// prompts stay in this process instead of a desktop applet.
class NmSecretAgent : public QDBusVirtualObject
{
    Q_OBJECT

public:
    enum class PairingKind {
        Pin,
        PushButton,
    };
    Q_ENUM(PairingKind)

    explicit NmSecretAgent(QObject *parent = nullptr);
    ~NmSecretAgent() override;

    bool registered() const;
    bool pairingPending() const;

    QString introspect(const QString &path) const override;
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;

public Q_SLOTS:
    void providePin(const QString &pin);
    void cancel();

Q_SIGNALS:
    void pairingRequested(NmSecretAgent::PairingKind kind, const QString &sinkName);
    void pairingFinished();

private:
    void unregisterAgent();
    void replyPendingError(const QString &name, const QString &message);
    bool handleGetSecrets(const QDBusMessage &message, const QDBusConnection &connection);

    QDBusConnection m_bus;
    QDBusMessage m_pending;
    bool m_registered = false;
    bool m_hasPending = false;
};
