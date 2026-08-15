#pragma once

#include <QByteArray>
#include <QString>

// AAC-LC stereo as carried in MPEG-TS. LPCM is not used on this mux path.
struct WfdAudioMode {
    enum class Codec { None, Aac };

    Codec codec = Codec::None;
    int rate = 48000;
    int channels = 2;

    bool enabled() const;
    QString description() const;
    // SET_PARAMETER / GET_PARAMETER value ("AAC 00000001 00" or "none").
    QByteArray codecsParameter() const;
};

QByteArray wfdSourceAudioParameter();

// Parse wfd_audio_codecs. If enabled is false, or the sink has no AAC, returns none.
WfdAudioMode selectWfdAudioMode(const QByteArray &getParameterBody, bool enabled);
