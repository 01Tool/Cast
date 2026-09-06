#include "session/wfdaudiomode.h"

#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>

namespace {

// AAC (WFD Table 5-18 / GNOME wfd-params): bit 0 = 48 kHz 2ch, bit 1 = 44.1 kHz 2ch.
constexpr quint32 kAac48k = 1u << 0;
constexpr quint32 kAac441k = 1u << 1;
// LPCM (WFD Table 5-17 / Intel wds / Android WifiDisplaySource): bit 0 = 44.1 kHz 2ch,
// bit 1 = 48 kHz 2ch. Opposite of AAC.
constexpr quint32 kLpcm441k = 1u << 0;
constexpr quint32 kLpcm48k = 1u << 1;

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

quint32 codecMask(const QByteArray &value, const char *name)
{
    const QRegularExpression re(QStringLiteral("(?:^|,)\\s*%1\\s+([0-9a-fA-F]+)")
                                    .arg(QLatin1String(name)),
                                QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(QString::fromLatin1(value));
    if (!match.hasMatch())
        return 0;
    bool ok = false;
    const quint32 mask = match.captured(1).toUInt(&ok, 16);
    return ok ? mask : 0;
}

QByteArray hexMode(quint32 mask)
{
    return QByteArray::number(mask, 16).rightJustified(8, '0') + " 00";
}

} // namespace

bool WfdAudioMode::enabled() const
{
    return (codec == Codec::Aac || codec == Codec::Lpcm) && rate > 0 && channels > 0;
}

QString WfdAudioMode::description() const
{
    if (!enabled())
        return QCoreApplication::translate("WfdAudioMode", "none");
    const QString name = (codec == Codec::Lpcm)
        ? QCoreApplication::translate("WfdAudioMode", "LPCM %1 kHz")
        : QCoreApplication::translate("WfdAudioMode", "AAC %1 kHz");
    return name.arg(rate / 1000);
}

QByteArray WfdAudioMode::codecsParameter() const
{
    if (!enabled())
        return QByteArrayLiteral("none");
    if (codec == Codec::Lpcm) {
        const quint32 mask = (rate == 44100) ? kLpcm441k : kLpcm48k;
        return QByteArray("LPCM ") + hexMode(mask);
    }
    const quint32 mask = (rate == 44100) ? kAac441k : kAac48k;
    return QByteArray("AAC ") + hexMode(mask);
}

QByteArray wfdSourceAudioParameter()
{
    return QByteArrayLiteral("AAC 00000001 00, LPCM 00000003 00");
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

    WfdAudioMode mode;
    mode.channels = 2;
    const quint32 aac = codecMask(value, "AAC");
    if (aac != 0) {
        mode.codec = WfdAudioMode::Codec::Aac;
        mode.rate = (aac & kAac48k) ? 48000 : 44100;
        qInfo() << "Selected WFD audio" << mode.description() << "from" << value;
        return mode;
    }

    const quint32 lpcm = codecMask(value, "LPCM");
    if (lpcm != 0) {
        mode.codec = WfdAudioMode::Codec::Lpcm;
        mode.rate = (lpcm & kLpcm48k) ? 48000 : 44100;
        qInfo() << "Selected WFD audio" << mode.description() << "from" << value;
        return mode;
    }

    qInfo() << "Sink has no AAC or LPCM in" << value << ", video only";
    return none;
}
