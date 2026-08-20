#pragma once

#include <QDBusInterface>
#include <QJsonArray>
#include <QObject>
#include <QString>
#include <memory>

class QDBusServiceWatcher;

class CastClient : public QObject
{
    Q_OBJECT

public:
    explicit CastClient(QObject *parent = nullptr);

    bool available() const;
    QString state() const;
    QString statusMessage() const;
    QJsonArray sinks() const;

public Q_SLOTS:
    void ensureService();
    void startScan();
    void connectToSink(const QString &id);
    void disconnectFromSink();
    void raiseWindow();

Q_SIGNALS:
    void availableChanged();
    void stateChanged();
    void sinksChanged();
    void errorOccurred(const QString &message);

private Q_SLOTS:
    void onRemoteChanged();

private:
    void bindInterface();
    void refresh();

    std::unique_ptr<QDBusInterface> m_iface;
    QDBusServiceWatcher *m_watcher = nullptr;
    bool m_available = false;
    QString m_state;
    QString m_status;
    QJsonArray m_sinks;
};
