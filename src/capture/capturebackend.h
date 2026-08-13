#pragma once

#include <QString>

class CaptureBackend
{
public:
    virtual ~CaptureBackend() = default;

    virtual QString name() const = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual QString lastError() const = 0;
};
