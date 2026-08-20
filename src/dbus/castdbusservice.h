#pragma once

#include "engine/castengine.h"

#include <QObject>
#include <QString>

class CastDBusService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.ot01tool.Cast1")
    Q_PROPERTY(QString State READ state NOTIFY stateChanged)
    Q_PROPERTY(QString StatusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString SelectedSinkId READ selectedSinkId NOTIFY selectedSinkIdChanged)

public:
    explicit CastDBusService(CastEngine *engine, QObject *parent = nullptr);

    QString state() const;
    QString statusMessage() const;
    QString selectedSinkId() const;
    bool registerService();

public Q_SLOTS:
    void StartScan();
    void StopScan();
    void Connect(const QString &sinkId);
    void Disconnect();
    void RaiseWindow();
    QString SinksJson() const;

Q_SIGNALS:
    void stateChanged(const QString &state);
    void statusMessageChanged(const QString &message);
    void selectedSinkIdChanged(const QString &id);
    void sinksChanged();
    void errorOccurred(const QString &message);
    void pairingRequested();
    void raiseRequested();

private:
    static QString stateName(CastEngine::SessionState state);

    CastEngine *m_engine = nullptr;
};