#include "session/wfdvideomode.h"

#include <QByteArray>
#include <cstdio>

static int g_failed = 0;

static void expectMode(const char *name, const WfdVideoMode &mode, int width, int height, int fps)
{
    if (mode.width == width && mode.height == height && mode.fps == fps)
        return;
    std::fprintf(stderr, "FAIL %s: got %dx%d@%d want %dx%d@%d\n",
                 name, mode.width, mode.height, mode.fps, width, height, fps);
    ++g_failed;
}

static void expectContains(const char *name, const QByteArray &hay, const char *needle)
{
    if (hay.contains(needle))
        return;
    std::fprintf(stderr, "FAIL %s: %s not in %s\n", name, needle, hay.constData());
    ++g_failed;
}

int main()
{
    expectMode("default", defaultWfdVideoMode(), 1280, 720, 30);
    expectMode("empty", selectWfdVideoMode({}), 1280, 720, 30);
    expectMode("none", selectWfdVideoMode("wfd_video_formats: none"), 1280, 720, 30);

    expectMode("cea 720p30",
               selectWfdVideoMode("wfd_video_formats: 00 00 01 01 00000020 00000000 00000000 00 0000 0000 00 none none"),
               1280, 720, 30);
    expectMode("prefer 1080p30 over 720p30",
               selectWfdVideoMode("wfd_video_formats: 00 00 01 01 000000a0 00000000 00000000 00 0000 0000 00 none none"),
               1920, 1080, 30);
    expectMode("prefer 720p30 over 720p60",
               selectWfdVideoMode("wfd_video_formats: 00 00 01 01 00000060 00000000 00000000 00 0000 0000 00 none none"),
               1280, 720, 30);
    expectMode("only 720p60",
               selectWfdVideoMode("wfd_video_formats: 00 00 01 01 00000040 00000000 00000000 00 0000 0000 00 none none"),
               1280, 720, 60);
    expectMode("vesa 1366x768p60",
               selectWfdVideoMode("00 00 01 01 00000000 00002000 00000000 00 0000 0000 00 none none"),
               1366, 768, 60);
    expectMode("max 1280x720 filters 1080p",
               selectWfdVideoMode("00 00 01 01 000000a0 00000000 00000000 00 0000 0000 00 1280 720"),
               1280, 720, 30);

    const WfdVideoMode picked = selectWfdVideoMode(
        "wfd_video_formats: 00 00 01 01 000000a0 00000000 00000000 00 0000 0000 00 none none");
    expectContains("single-bit set", picked.formatsParameter(), "00000080");

    if (g_failed) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failed);
        return 1;
    }
    std::puts("wfdvideomode-check ok");
    return 0;
}
