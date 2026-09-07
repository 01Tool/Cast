#include "capture/screencastportal.h"

#include <cstdio>

static int g_failed = 0;

static void expectEq(const char *name, const QString &got, const QString &want)
{
    if (got == want)
        return;
    std::fprintf(stderr, "FAIL %s: got [%s] want [%s]\n", name, qPrintable(got), qPrintable(want));
    ++g_failed;
}

int main()
{
    expectEq("request :1.42",
             ScreenCastPortal::objectPath(QStringLiteral("request"), QStringLiteral(":1.42"),
                                          QStringLiteral("cast1")),
             QStringLiteral("/org/freedesktop/portal/desktop/request/1_42/cast1"));
    expectEq("session dotted unique",
             ScreenCastPortal::objectPath(QStringLiteral("session"), QStringLiteral(":1.138"),
                                          QStringLiteral("cast00aabb")),
             QStringLiteral("/org/freedesktop/portal/desktop/session/1_138/cast00aabb"));
    expectEq("no leading colon",
             ScreenCastPortal::objectPath(QStringLiteral("request"), QStringLiteral("1.2.3"),
                                          QStringLiteral("t")),
             QStringLiteral("/org/freedesktop/portal/desktop/request/1_2_3/t"));

    if (g_failed) {
        std::fprintf(stderr, "%d portal-check test(s) failed\n", g_failed);
        return 1;
    }
    return 0;
}
