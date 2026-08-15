#pragma once

#include "capture/capturebackend.h"

#include <QCoreApplication>

class X11Capture : public CaptureBackend
{
    Q_DECLARE_TR_FUNCTIONS(X11Capture)
public:
    QString name() const override;
    bool start(const DisplaySource &source) override;
    void stop() override;
    QString lastError() const override;

private:
    QString m_lastError;
    DisplaySource m_source;
    bool m_running = false;
};
