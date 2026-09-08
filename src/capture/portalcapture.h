#pragma once

#include "capture/capturebackend.h"

#include <QObject>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QString>
#include <QTimer>
#include <QVariantMap>

// Experimental Treeland path. Start/OpenPipeWireRemote must not block the GUI
// thread; see docs/platform/wayland.md §Treeland smoke.
class PortalCapture : public QObject, public CaptureBackend
{
    Q_OBJECT
public:
    explicit PortalCapture(QObject *parent = nullptr);

    QString name() const override;
    bool start(const DisplaySource &source) override;
    void stop() override;
    QString lastError() const override;
    int pipewireFd() const override;
    uint pipewireNode() const override;
    int streamWidth() const override;
    int streamHeight() const override;

Q_SIGNALS:
    void ready();
    void failed(const QString &message);

private Q_SLOTS:
    void onStartInvoked(QDBusPendingCallWatcher *watcher);
    void onStartMessage(const QDBusMessage &message);
    void onStartTimeout();
    void completeStart();
    void onPipeWireRemote(QDBusPendingCallWatcher *watcher);

private:
    bool createSession();
    bool selectSources(uint cursorModes);
    bool beginStartSession();
    bool openPipeWireRemote();
    bool callRequest(const QString &method, const QVariantList &args, int timeoutMs,
                     QVariantMap *results, const QVariantMap &extraOptions = {});
    uint availableSourceTypes() const;
    uint availableCursorModes() const;
    uint readPortalUintProperty(const QString &name) const;
    void unwatchStartResponse();
    void closeSession();
    void failStart(const QString &message);
    void finishStart(uint response, const QVariantMap &results);
    bool watchResponse(const QString &path, const char *slot);

    QString m_lastError;
    QDBusObjectPath m_session;
    QString m_startRequestPath;
    QTimer m_startTimer;
    bool m_starting = false;
    int m_pipewireFd = -1;
    uint m_pipewireNode = 0;
    int m_streamWidth = 0;
    int m_streamHeight = 0;
};
