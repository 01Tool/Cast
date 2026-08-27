#include "capture/displaysource.h"

#include <cstdio>

static int g_failed = 0;

static void expectEq(const char *name, const QString &got, const QString &want)
{
    if (got == want)
        return;
    std::fprintf(stderr, "FAIL %s: got \"%s\" want \"%s\"\n", name, qPrintable(got),
                 qPrintable(want));
    ++g_failed;
}

static void expectTrue(const char *name, bool ok)
{
    if (ok)
        return;
    std::fprintf(stderr, "FAIL %s\n", name);
    ++g_failed;
}

int main()
{
    DisplaySource empty;
    expectTrue("empty invalid", !empty.isValid());
    expectEq("empty region", ximagesrcRegionProperties(empty), {});
    expectEq("empty size", x11grabSize(empty), {});
    expectEq("empty input", x11grabInputSpecifier(QStringLiteral(":0"), empty),
             QStringLiteral(":0"));

    DisplaySource primary;
    primary.id = QStringLiteral("HDMI-1");
    primary.x = 1920;
    primary.y = 0;
    primary.width = 2560;
    primary.height = 1440;
    expectTrue("primary valid", primary.isValid());
    expectEq("short name", primary.shortName(), QStringLiteral("HDMI-1"));
    expectEq("ximagesrc crop", ximagesrcRegionProperties(primary),
             QStringLiteral("startx=1920 starty=0 endx=4479 endy=1439"));
    expectEq("ffmpeg size", x11grabSize(primary), QStringLiteral("2560x1440"));
    expectEq("ffmpeg input", x11grabInputSpecifier(QStringLiteral(":0"), primary),
             QStringLiteral(":0+1920,0"));

    const QRect hidpi = scaleToNativePixels(QRect(0, 0, 1920, 1080), 2.0);
    expectTrue("hidpi 4k width", hidpi.width() == 3840 && hidpi.height() == 2160);
    const QRect right = scaleToNativePixels(QRect(1920, 0, 1920, 1080), 2.0);
    expectTrue("hidpi origin", right.x() == 3840 && right.width() == 3840);
    const QRect unscaled = scaleToNativePixels(QRect(0, 0, 3840, 2160), 1.0);
    expectTrue("dpr 1 unchanged", unscaled.width() == 3840 && unscaled.height() == 2160);

    if (g_failed) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failed);
        return 1;
    }
    std::puts("displaysource-check ok");
    return 0;
}
