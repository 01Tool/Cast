#pragma once

#include <QByteArray>
#include <QtGlobal>

// Treeland is DDE’s compositor. DTK maps IsWaylandPlatform to Treeland window
// chrome (treeland_personalization), which is not capture. Detect the socket
// and session name; do not fold Treeland into the generic Wayland backend.
inline bool isTreelandSession(const QByteArray &waylandDisplay,
                              const QByteArray &desktopSession,
                              const QByteArray &xdgSessionDesktop = {})
{
    const auto mentionsTreeland = [](const QByteArray &value) {
        return value.toLower().contains("treeland");
    };
    return mentionsTreeland(waylandDisplay) || mentionsTreeland(desktopSession)
        || mentionsTreeland(xdgSessionDesktop);
}

inline bool isTreelandSession()
{
    return isTreelandSession(qgetenv("WAYLAND_DISPLAY"), qgetenv("DESKTOP_SESSION"),
                             qgetenv("XDG_SESSION_DESKTOP"));
}
