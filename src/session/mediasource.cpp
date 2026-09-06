#include "session/mediasource.h"

#include <QFileInfo>
#include <QHash>

namespace {

QString mimeForSuffix(QString suffix)
{
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("jpg"), QStringLiteral("image/jpeg")},
        {QStringLiteral("jpeg"), QStringLiteral("image/jpeg")},
        {QStringLiteral("png"), QStringLiteral("image/png")},
        {QStringLiteral("gif"), QStringLiteral("image/gif")},
        {QStringLiteral("webp"), QStringLiteral("image/webp")},
        {QStringLiteral("bmp"), QStringLiteral("image/bmp")},
        {QStringLiteral("mp4"), QStringLiteral("video/mp4")},
        {QStringLiteral("m4v"), QStringLiteral("video/mp4")},
        {QStringLiteral("mov"), QStringLiteral("video/quicktime")},
        {QStringLiteral("mkv"), QStringLiteral("video/x-matroska")},
        {QStringLiteral("webm"), QStringLiteral("video/webm")},
        {QStringLiteral("avi"), QStringLiteral("video/x-msvideo")},
        {QStringLiteral("wmv"), QStringLiteral("video/x-ms-wmv")},
        {QStringLiteral("ts"), QStringLiteral("video/mpeg")},
        {QStringLiteral("m2ts"), QStringLiteral("video/mpeg")},
        {QStringLiteral("mpeg"), QStringLiteral("video/mpeg")},
        {QStringLiteral("mpg"), QStringLiteral("video/mpeg")},
        {QStringLiteral("mp3"), QStringLiteral("audio/mpeg")},
        {QStringLiteral("m4a"), QStringLiteral("audio/mp4")},
        {QStringLiteral("aac"), QStringLiteral("audio/aac")},
        {QStringLiteral("wav"), QStringLiteral("audio/wav")},
        {QStringLiteral("flac"), QStringLiteral("audio/flac")},
        {QStringLiteral("ogg"), QStringLiteral("audio/ogg")},
        {QStringLiteral("oga"), QStringLiteral("audio/ogg")},
    };
    suffix = suffix.toLower();
    return kMap.value(suffix, QStringLiteral("application/octet-stream"));
}

MediaKind kindForMime(const QString &mime)
{
    if (mime.startsWith(QLatin1String("image/")))
        return MediaKind::Image;
    if (mime.startsWith(QLatin1String("audio/")))
        return MediaKind::Audio;
    if (mime.startsWith(QLatin1String("video/")))
        return MediaKind::Video;
    return MediaKind::Video;
}

} // namespace

MediaSource mediaSourceFromPath(const QString &path)
{
    MediaSource media;
    if (path.isEmpty())
        return media;
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable())
        return media;
    media.path = info.absoluteFilePath();
    media.title = info.fileName();
    media.size = info.size();
    media.mime = mimeForSuffix(info.suffix());
    media.kind = kindForMime(media.mime);
    return media;
}

QString upnpClassForMedia(MediaKind kind)
{
    switch (kind) {
    case MediaKind::Image:
        return QStringLiteral("object.item.imageItem.photo");
    case MediaKind::Audio:
        return QStringLiteral("object.item.audioItem.musicTrack");
    case MediaKind::Video:
        return QStringLiteral("object.item.videoItem");
    case MediaKind::Monitor:
        break;
    }
    return QStringLiteral("object.item.videoItem");
}

QString mediaKindKey(MediaKind kind)
{
    switch (kind) {
    case MediaKind::Image:
        return QStringLiteral("image");
    case MediaKind::Audio:
        return QStringLiteral("audio");
    case MediaKind::Video:
        return QStringLiteral("video");
    case MediaKind::Monitor:
        break;
    }
    return QStringLiteral("monitor");
}

HttpByteRange parseHttpByteRange(const QByteArray &headerValue, qint64 size)
{
    HttpByteRange range;
    if (size < 0)
        size = 0;
    range.end = size > 0 ? size - 1 : -1;
    const QByteArray value = headerValue.trimmed();
    if (value.isEmpty())
        return range;
    range.specified = true;
    if (!value.toLower().startsWith("bytes=")) {
        range.valid = false;
        return range;
    }
    const QByteArray spec = value.mid(6).trimmed();
    const int dash = spec.indexOf('-');
    if (dash < 0) {
        range.valid = false;
        return range;
    }
    const QByteArray startTok = spec.left(dash).trimmed();
    const QByteArray endTok = spec.mid(dash + 1).trimmed();
    bool okStart = true;
    bool okEnd = true;
    if (startTok.isEmpty()) {
        // suffix: bytes=-500
        const qint64 suffix = endTok.toLongLong(&okEnd);
        if (!okEnd || suffix <= 0) {
            range.valid = false;
            return range;
        }
        range.start = qMax(qint64(0), size - suffix);
        range.end = size - 1;
        return range;
    }
    range.start = startTok.toLongLong(&okStart);
    if (endTok.isEmpty())
        range.end = size - 1;
    else
        range.end = endTok.toLongLong(&okEnd);
    if (!okStart || !okEnd || range.start < 0 || range.end < range.start || range.start >= size) {
        range.valid = false;
        return range;
    }
    if (range.end >= size)
        range.end = size - 1;
    return range;
}
