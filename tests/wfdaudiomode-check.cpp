#include "session/wfdaudiomode.h"

#include <cstdio>

static int g_failed = 0;

static void expectNone(const char *name, const WfdAudioMode &mode)
{
    if (!mode.enabled())
        return;
    std::fprintf(stderr, "FAIL %s: expected none, got %s\n", name,
                 qPrintable(mode.description()));
    ++g_failed;
}

static void expectAac(const char *name, const WfdAudioMode &mode, int rate)
{
    if (mode.enabled() && mode.codec == WfdAudioMode::Codec::Aac && mode.rate == rate)
        return;
    std::fprintf(stderr, "FAIL %s: got enabled=%d rate=%d want AAC %d\n", name,
                 int(mode.enabled()), mode.rate, rate);
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
    expectNone("disabled", selectWfdAudioMode("wfd_audio_codecs: AAC 00000001 00", false));
    expectNone("none", selectWfdAudioMode("wfd_audio_codecs: none", true));
    expectNone("lpcm only",
               selectWfdAudioMode("wfd_audio_codecs: LPCM 00000003 00", true));
    expectAac("aac 48k",
              selectWfdAudioMode("wfd_audio_codecs: AAC 00000001 00", true), 48000);
    expectAac("aac after lpcm",
              selectWfdAudioMode("wfd_audio_codecs: LPCM 00000003 00, AAC 00000001 00", true),
              48000);
    expectAac("aac 44.1 only",
              selectWfdAudioMode("wfd_audio_codecs: AAC 00000002 00", true), 44100);
    expectAac("prefer 48k when both",
              selectWfdAudioMode("wfd_audio_codecs: AAC 00000003 00", true), 48000);

    const WfdAudioMode aac48 = selectWfdAudioMode("AAC 00000001 00", true);
    expectContains("set-parameter 48k", aac48.codecsParameter(), "00000001");
    expectContains("source caps", wfdSourceAudioParameter(), "AAC 00000001 00");

    if (g_failed) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failed);
        return 1;
    }
    std::puts("wfdaudiomode-check ok");
    return 0;
}
