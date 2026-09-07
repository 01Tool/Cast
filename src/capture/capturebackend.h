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

    // PipeWire remote from xdg-desktop-portal ScreenCast. -1 / 0 on X11.
    virtual int pipewireFd() const { return -1; }
    virtual uint pipewireNode() const { return 0; }
    virtual int streamWidth() const { return 0; }
    virtual int streamHeight() const { return 0; }
};
