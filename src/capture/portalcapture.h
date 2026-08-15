#pragma once

#include "capture/capturebackend.h"

class PortalCapture : public CaptureBackend
{
public:
    QString name() const override;
    bool start(const DisplaySource &source) override;
    void stop() override;
    QString lastError() const override;

private:
    QString m_lastError;
};
