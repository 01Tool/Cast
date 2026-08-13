#pragma once

#include "capture/capturebackend.h"

class X11Capture : public CaptureBackend
{
public:
    QString name() const override;
    bool start() override;
    void stop() override;
    QString lastError() const override;

private:
    QString m_lastError;
    bool m_running = false;
};
