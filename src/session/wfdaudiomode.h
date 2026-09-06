#pragma once

#include <QByteArray>
#include <QString>

// AAC-LC or LPCM stereo in MPEG-TS. Prefer AAC when the sink lists it.
struct WfdAudioMode {
    enum class Codec { None, Aac, Lpcm };

    Codec codec = Codec::None;
    int rate = 48000;
    int channels = 2;

    bool enabled() const;
    QString description() const;
    // SET_PARAMETER / GET_PARAMETER value ("AAC 00000001 00", "LPCM 00000002 00", or "none").
    QByteArray codecsParameter() const;
};

QByteArray wfdSourceAudioParameter();

// Parse wfd_audio_codecs. Prefer AAC, else LPCM. None if audio is off or the sink lists neither.
WfdAudioMode selectWfdAudioMode(const QByteArray &getParameterBody, bool enabled);
