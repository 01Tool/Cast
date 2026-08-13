#include "capture/x11capture.h"

#include <QGuiApplication>

QString X11Capture::name() const
{
    return QStringLiteral("X11Capture");
}

bool X11Capture::start()
{
    if (qEnvironmentVariableIsEmpty("DISPLAY")) {
        m_lastError = QStringLiteral("DISPLAY is not set; cannot grab the X11 screen.");
        m_running = false;
        return false;
    }
    m_lastError.clear();
    m_running = true;
    return true;
}

void X11Capture::stop()
{
    m_running = false;
}

QString X11Capture::lastError() const
{
    return m_lastError;
}
