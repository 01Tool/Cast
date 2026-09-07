#pragma once

#include <QString>

// xdg-desktop-portal ScreenCast object paths.
// Request/session handles are /org/freedesktop/portal/desktop/{request,session}/SENDER/TOKEN
// where SENDER is the unique name without a leading ':' and with '.' replaced by '_'.
namespace ScreenCastPortal {

constexpr auto desktopService = "org.freedesktop.portal.Desktop";
constexpr auto desktopPath = "/org/freedesktop/portal/desktop";
constexpr auto screenCastInterface = "org.freedesktop.portal.ScreenCast";
constexpr auto requestInterface = "org.freedesktop.portal.Request";
constexpr auto sessionInterface = "org.freedesktop.portal.Session";
constexpr auto propertiesInterface = "org.freedesktop.DBus.Properties";

// MONITOR in org.freedesktop.portal.ScreenCast AvailableSourceTypes / SelectSources types.
constexpr uint monitorSource = 1;
// Embedded cursor (drawn in the stream).
constexpr uint cursorEmbedded = 2;
// gst-launch child inherits the PipeWire remote as this fd.
constexpr int gstPipeWireFd = 3;

inline QString objectPath(const QString &kind, const QString &uniqueName, const QString &token)
{
    QString sender = uniqueName;
    if (sender.startsWith(QLatin1Char(':')))
        sender.remove(0, 1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    return QStringLiteral("/org/freedesktop/portal/desktop/%1/%2/%3").arg(kind, sender, token);
}

} // namespace ScreenCastPortal
