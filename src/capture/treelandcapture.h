#pragma once

#include "capture/capturebackend.h"

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QString>
#include <QTimer>
#include <QVariantMap>

struct DBusConnection;
class QSocketNotifier;

// Treeland ScreenCast path. Start/OpenPipeWireRemote must not block the GUI
// thread; see docs/platform/treeland.md. Still org.freedesktop.portal.ScreenCast,
// not Treeland compositor protocols.
//
// Do not QDBus-connect Request.Response for Start: Qt demarshals ua{sv} into
// QVariantMap (including streams a(ua{sv})) and hangs after Allow. A private
// libdbus connection BecomeMonitor's that unicast signal instead.
class TreelandCapture : public QObject, public CaptureBackend
{
    Q_OBJECT
public:
    explicit TreelandCapture(QObject *parent = nullptr);
    ~TreelandCapture() override;

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
    void onStartTimeout();
    void onMonitorSocket();
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
    bool startResponseWatcher(const QString &token);
    void stopResponseWatcher();
    void closeSession();
    void failStart(const QString &message);
    void finishStart(uint response, uint node, int width, int height);
    void handleMonitorMessage(void *dbusMessage);

    QString m_lastError;
    QDBusObjectPath m_session;
    QString m_startToken;
    QTimer m_startTimer;
    DBusConnection *m_monitor = nullptr;
    QSocketNotifier *m_monitorNotifier = nullptr;
    bool m_starting = false;
    int m_pipewireFd = -1;
    uint m_pipewireNode = 0;
    int m_streamWidth = 0;
    int m_streamHeight = 0;
};
