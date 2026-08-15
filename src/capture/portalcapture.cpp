#include "capture/portalcapture.h"

QString PortalCapture::name() const
{
    return QStringLiteral("PortalCapture");
}

bool PortalCapture::start(const DisplaySource &source)
{
    Q_UNUSED(source);
    // Do not fall back to X11 grab on Wayland (XWayland-only frames).
    m_lastError = QStringLiteral(
        "Screen capture is unavailable on this session. "
        "xdg-desktop-portal ScreenCast is not available.");
    return false;
}

void PortalCapture::stop()
{
}

QString PortalCapture::lastError() const
{
    return m_lastError;
}
