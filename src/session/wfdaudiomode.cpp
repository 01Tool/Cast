#include "session/wfdaudiomode.h"

#include <QDebug>
#include <QRegularExpression>

namespace {

// Wi-Fi Display AAC bitmap (GNOME wfd-params / WFD spec Table 5-18):
// bit 0 = 48 kHz 16-bit 2ch, bit 1 = 44.1 kHz 16-bit 2ch.
constexpr quint32 kAac48k = 1u << 0;
constexpr quint32 kAac441k = 1u << 1;

QByteArray audioCodecsValue(const QByteArray &body)
{
    const QByteArray prefix("wfd_audio_codecs:");
    for (QByteArray line : body.split('\n')) {
        line = line.trimmed();
        if (line.startsWith(prefix))
            return line.mid(prefix.size()).trimmed();
    }
    if (body.contains("wfd_"))
        return {};
    return body.trimmed();
}

} // namespace

bool WfdAudioMode::enabled() const
{
    return codec == Codec::Aac && rate > 0 && channels > 0;
}

QString WfdAudioMode::description() const
{
    if (!enabled())
        return QStringLiteral("none");
    return QStringLiteral("AAC %1 kHz").arg(rate / 1000);
}

QByteArray WfdAudioMode::codecsParameter() const
{
    if (!enabled())
        return QByteArrayLiteral("none");
    const quint32 mask = (rate == 44100) ? kAac441k : kAac48k;
    return QByteArray("AAC ") + QByteArray::number(mask, 16).rightJustified(8, '0') + " 00";
}

QByteArray wfdSourceAudioParameter()
{
    return QByteArrayLiteral("AAC 00000001 00");
}

WfdAudioMode selectWfdAudioMode(const QByteArray &getParameterBody, bool enabled)
{
    WfdAudioMode none;
    if (!enabled)
        return none;

    const QByteArray value = audioCodecsValue(getParameterBody);
    if (value.isEmpty() || value == "none") {
        qInfo() << "Sink audio codecs missing or none, video only";
        return none;
    }

    static const QRegularExpression aacRe(
        QStringLiteral("(?:^|,)\\s*AAC\\s+([0-9a-fA-F]+)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = aacRe.match(QString::fromLatin1(value));
    if (!match.hasMatch()) {
        qInfo() << "Sink has no AAC in" << value << ", video only";
        return none;
    }

    bool ok = false;
    const quint32 mask = match.captured(1).toUInt(&ok, 16);
    if (!ok || mask == 0) {
        qInfo() << "Sink AAC bitmap empty in" << value << ", video only";
        return none;
    }

    WfdAudioMode mode;
    mode.codec = WfdAudioMode::Codec::Aac;
    mode.channels = 2;
    mode.rate = (mask & kAac48k) ? 48000 : 44100;
    qInfo() << "Selected WFD audio" << mode.description() << "from" << value;
    return mode;
}
