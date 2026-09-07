#include "dbus/castdbus.h"

#include <QFile>
#include <cstdio>
#include <unistd.h>

static int g_failed = 0;

static void expectTrue(const char *name, bool ok)
{
    if (ok)
        return;
    std::fprintf(stderr, "FAIL %s\n", name);
    ++g_failed;
}

static void expectEq(const char *name, const QString &got, const QString &want)
{
    if (got == want)
        return;
    std::fprintf(stderr, "FAIL %s: got [%s] want [%s]\n", name, qPrintable(got), qPrintable(want));
    ++g_failed;
}

int main()
{
    const QString self = QFile::symLinkTarget(QStringLiteral("/proc/self/exe"));
    expectTrue("proc self exe", !self.isEmpty());
    expectEq("peerExecutable self", CastDBus::peerExecutable(::getpid()), self);
    expectTrue("peerExecutable zero", CastDBus::peerExecutable(0).isEmpty());
    expectTrue("peerExecutable negative", CastDBus::peerExecutable(-1).isEmpty());

    expectTrue("exact self", CastDBus::controlCallerAllowed(self, self));
    expectTrue("ot-cast exact",
               CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/ot-cast"),
                                              QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("build-dir self",
               CastDBus::controlCallerAllowed(QStringLiteral("/home/x/build/ot-cast"),
                                              QStringLiteral("/home/x/build/ot-cast")));
    expectTrue("deleted self",
               CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/ot-cast (deleted)"),
                                              QStringLiteral("/usr/bin/ot-cast")));

    expectTrue("dde-shell",
               CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/dde-shell"),
                                              QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("dde-dock",
               CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/dde-dock"),
                                              QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("dde-tray-loader",
               CastDBus::controlCallerAllowed(QStringLiteral("/usr/libexec/dde-tray-loader"),
                                              QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deleted dde-shell",
               CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/dde-shell (deleted)"),
                                              QStringLiteral("/usr/bin/ot-cast")));

    expectTrue("deny empty",
               !CastDBus::controlCallerAllowed({}, QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deny gdbus",
               !CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/gdbus"),
                                               QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deny dbus-send",
               !CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/dbus-send"),
                                               QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deny python",
               !CastDBus::controlCallerAllowed(QStringLiteral("/usr/bin/python3"),
                                               QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deny tmp ot-cast name",
               !CastDBus::controlCallerAllowed(QStringLiteral("/tmp/ot-cast"),
                                               QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deny tmp dde-shell",
               !CastDBus::controlCallerAllowed(QStringLiteral("/tmp/dde-shell"),
                                               QStringLiteral("/usr/bin/ot-cast")));
    expectTrue("deny home dde-dock",
               !CastDBus::controlCallerAllowed(QStringLiteral("/home/x/dde-dock"),
                                               QStringLiteral("/usr/bin/ot-cast")));

    if (g_failed) {
        std::fprintf(stderr, "%d dbus-check test(s) failed\n", g_failed);
        return 1;
    }
    return 0;
}
