#include "dbus/castdbus.h"

#include <QFile>
#include <QFileInfo>

namespace CastDBus {
namespace {

QString stripDeletedSuffix(const QString &path)
{
    static const QString suffix = QStringLiteral(" (deleted)");
    if (path.endsWith(suffix))
        return path.left(path.size() - suffix.size());
    return path;
}

bool isSystemTrayHost(const QString &exe)
{
    const QString base = QFileInfo(exe).fileName();
    if (base != QLatin1String("dde-shell") && base != QLatin1String("dde-dock")
        && base != QLatin1String("dde-tray-loader")) {
        return false;
    }
    return exe.startsWith(QLatin1String("/usr/bin/"))
        || exe.startsWith(QLatin1String("/usr/libexec/"));
}

} // namespace

QString peerExecutable(qint64 pid)
{
    if (pid <= 0)
        return {};
    return QFile::symLinkTarget(QStringLiteral("/proc/%1/exe").arg(pid));
}

bool controlCallerAllowed(const QString &exePath, const QString &selfExePath)
{
    const QString exe = stripDeletedSuffix(exePath);
    if (exe.isEmpty())
        return false;

    const QString self = stripDeletedSuffix(selfExePath);
    if (!self.isEmpty() && exe == self)
        return true;

    return isSystemTrayHost(exe);
}

} // namespace CastDBus
