#pragma once

#include "capture/capturebackend.h"

#include <QCoreApplication>

class PortalCapture : public CaptureBackend
{
    Q_DECLARE_TR_FUNCTIONS(PortalCapture)
public:
    QString name() const override;
    bool start(const DisplaySource &source) override;
    void stop() override;
    QString lastError() const override;

private:
    QString m_lastError;
};
