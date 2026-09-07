#pragma once

#include "capture/capturebackend.h"

#include <QCoreApplication>
#include <QDBusObjectPath>
#include <QString>

class PortalCapture : public CaptureBackend
{
    Q_DECLARE_TR_FUNCTIONS(PortalCapture)
public:
    QString name() const override;
    bool start(const DisplaySource &source) override;
    void stop() override;
    QString lastError() const override;
    int pipewireFd() const override;
    uint pipewireNode() const override;
    int streamWidth() const override;
    int streamHeight() const override;

private:
    bool createSession();
    bool selectSources();
    bool startSession();
    bool openPipeWireRemote();
    bool callRequest(const QString &method, const QVariantList &args, int timeoutMs,
                     QVariantMap *results);
    uint availableSourceTypes() const;
    void closeSession();

    QString m_lastError;
    QDBusObjectPath m_session;
    int m_pipewireFd = -1;
    uint m_pipewireNode = 0;
    int m_streamWidth = 0;
    int m_streamHeight = 0;
};
