#include "capture/x11capture.h"

QString X11Capture::name() const
{
    return QStringLiteral("X11Capture");
}

bool X11Capture::start()
{
    // Frame grab (ximagesrc / XShm) is wired in a later cut.
    m_lastError = QStringLiteral("X11 capture pipeline is not wired yet.");
    m_running = false;
    return false;
}

void X11Capture::stop()
{
    m_running = false;
}

QString X11Capture::lastError() const
{
    return m_lastError;
}
