#pragma once

#include <QString>

namespace CastDBus {

constexpr auto service = "com.ot01tool.Cast";
constexpr auto path = "/com/ot01tool/Cast";
constexpr auto interface = "com.ot01tool.Cast1";

// Resolve /proc/<pid>/exe. Empty if pid is invalid or the link is unreadable.
QString peerExecutable(qint64 pid);

// True if this D-Bus peer may call Cast control methods.
// The running ot-cast binary (exact path) and the DDE tray host
// (dde-shell / dde-dock / dde-tray-loader under /usr/bin or /usr/libexec).
bool controlCallerAllowed(const QString &exePath, const QString &selfExePath);

} // namespace CastDBus
