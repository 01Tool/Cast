#pragma once

#include "capture/displaysource.h"

#include <QString>

class CaptureBackend
{
public:
    virtual ~CaptureBackend() = default;

    virtual QString name() const = 0;
    virtual bool start(const DisplaySource &source) = 0;
    virtual void stop() = 0;
    virtual QString lastError() const = 0;
};
