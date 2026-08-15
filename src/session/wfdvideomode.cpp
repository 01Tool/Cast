#include "session/wfdvideomode.h"

#include <QDebug>
#include <QList>

namespace {

struct ResolutionBit {
    int bit;
    int width;
    int height;
    int fps;
    bool interlaced;
};

// Wi-Fi Display CEA / VESA / HH bit maps, matching GNOME Network Displays
// src/wfd/wfd-params.c (resolution_table).
constexpr ResolutionBit kCea[] = {
    {0, 640, 480, 60, false},
    {1, 720, 480, 60, false},
    {2, 720, 480, 60, true},
    {3, 720, 576, 50, false},
    {4, 720, 576, 50, true},
    {5, 1280, 720, 30, false},
    {6, 1280, 720, 60, false},
    {7, 1920, 1080, 30, false},
    {8, 1920, 1080, 60, false},
    {9, 1920, 1080, 60, true},
    {10, 1280, 720, 25, false},
    {11, 1280, 720, 50, false},
    {12, 1920, 1080, 25, false},
    {13, 1920, 1080, 50, false},
    {14, 1920, 1080, 50, true},
    {15, 1280, 720, 24, false},
    {16, 1920, 1080, 24, false},
};

constexpr ResolutionBit kVesa[] = {
    {0, 800, 600, 30, false},
    {1, 800, 600, 60, false},
    {2, 1024, 768, 30, false},
    {3, 1024, 768, 60, false},
    {4, 1152, 864, 30, false},
    {5, 1152, 864, 60, false},
    {6, 1280, 768, 30, false},
    {7, 1280, 768, 60, false},
    {8, 1280, 800, 30, false},
    {9, 1280, 800, 60, false},
    {10, 1360, 768, 30, false},
    {11, 1360, 768, 60, false},
    {12, 1366, 768, 30, false},
    {13, 1366, 768, 60, false},
    {14, 1280, 1024, 30, false},
    {15, 1280, 1024, 60, false},
    {16, 1400, 1050, 30, false},
    {17, 1400, 1050, 60, false},
    {18, 1440, 900, 30, false},
    {19, 1440, 900, 60, false},
    {20, 1600, 900, 30, false},
    {21, 1600, 900, 60, false},
    {22, 1600, 1200, 30, false},
    {23, 1600, 1200, 60, false},
    {24, 1680, 1024, 30, false},
    {25, 1680, 1024, 60, false},
    {26, 1680, 1050, 30, false},
    {27, 1680, 1050, 60, false},
    {28, 1920, 1200, 30, false},
    {29, 1920, 1200, 60, false},
};

constexpr ResolutionBit kHh[] = {
    {0, 800, 480, 30, false},
    {1, 800, 480, 60, false},
    {2, 854, 480, 30, false},
    {3, 854, 480, 60, false},
    {4, 864, 480, 30, false},
    {5, 864, 480, 60, false},
    {6, 640, 360, 30, false},
    {7, 640, 360, 60, false},
    {8, 960, 540, 30, false},
    {9, 960, 540, 60, false},
    {10, 848, 480, 30, false},
    {11, 848, 480, 60, false},
};

struct Candidate {
    WfdVideoMode::Table table;
    const ResolutionBit *res;
};

QByteArray hex32(quint32 value)
{
    return QByteArray::number(value, 16).rightJustified(8, '0');
}

QByteArray formatsLine(quint32 cea, quint32 vesa, quint32 hh)
{
    return QByteArray("00 00 01 01 ") + hex32(cea) + ' ' + hex32(vesa) + ' ' + hex32(hh)
        + " 00 0000 0000 00 none none";
}

template<size_t N>
quint32 sourceMask(const ResolutionBit (&table)[N])
{
    quint32 mask = 0;
    for (const auto &res : table) {
        if (!res.interlaced)
            mask |= (quint32(1) << res.bit);
    }
    return mask;
}

template<size_t N>
void collect(QList<Candidate> *out,
             WfdVideoMode::Table table,
             const ResolutionBit (&bits)[N],
             quint32 mask,
             int maxWidth,
             int maxHeight,
             bool progressiveOnly)
{
    for (const auto &res : bits) {
        if ((mask & (quint32(1) << res.bit)) == 0)
            continue;
        if (progressiveOnly && res.interlaced)
            continue;
        if (maxWidth > 0 && res.width > maxWidth)
            continue;
        if (maxHeight > 0 && res.height > maxHeight)
            continue;
        out->append({table, &res});
    }
}

// Prefer progressive, then ≤30 fps (software encode), then larger frames.
bool betterThan(const Candidate &a, const Candidate &b)
{
    if (a.res->interlaced != b.res->interlaced)
        return !a.res->interlaced;
    const bool aLow = a.res->fps <= 30;
    const bool bLow = b.res->fps <= 30;
    if (aLow != bLow)
        return aLow;
    const int aPixels = a.res->width * a.res->height;
    const int bPixels = b.res->width * b.res->height;
    if (aPixels != bPixels)
        return aPixels > bPixels;
    return a.res->fps > b.res->fps;
}

WfdVideoMode fromCandidate(const Candidate &c)
{
    WfdVideoMode mode;
    mode.width = c.res->width;
    mode.height = c.res->height;
    mode.fps = c.res->fps;
    mode.table = c.table;
    mode.bit = c.res->bit;
    return mode;
}

QByteArray formatsValue(const QByteArray &body)
{
    const QList<QByteArray> lines = body.split('\n');
    const QByteArray prefix("wfd_video_formats:");
    for (QByteArray line : lines) {
        line = line.trimmed();
        if (line.startsWith(prefix))
            return line.mid(prefix.size()).trimmed();
    }
    // Bare value (already just the tokens).
    if (body.contains("wfd_"))
        return {};
    return body.trimmed();
}

bool parseHex32(const QByteArray &token, quint32 *out)
{
    bool ok = false;
    const quint32 value = token.toUInt(&ok, 16);
    if (!ok)
        return false;
    *out = value;
    return true;
}

bool parseDec(const QByteArray &token, int *out)
{
    if (token == "none") {
        *out = 0;
        return true;
    }
    bool ok = false;
    const int value = token.toInt(&ok, 10);
    if (!ok || value < 0)
        return false;
    *out = value;
    return true;
}

} // namespace

bool WfdVideoMode::isValid() const
{
    return width > 0 && height > 0 && fps > 0 && bit >= 0 && bit < 32;
}

QString WfdVideoMode::description() const
{
    return QStringLiteral("%1x%2@%3").arg(width).arg(height).arg(fps);
}

QByteArray WfdVideoMode::formatsParameter() const
{
    quint32 cea = 0;
    quint32 vesa = 0;
    quint32 hh = 0;
    const quint32 bitMask = (quint32(1) << bit);
    switch (table) {
    case Table::Cea:
        cea = bitMask;
        break;
    case Table::Vesa:
        vesa = bitMask;
        break;
    case Table::Hh:
        hh = bitMask;
        break;
    }
    return formatsLine(cea, vesa, hh);
}

WfdVideoMode defaultWfdVideoMode()
{
    return {};
}

QByteArray wfdSourceFormatsParameter()
{
    return formatsLine(sourceMask(kCea), sourceMask(kVesa), sourceMask(kHh));
}

WfdVideoMode selectWfdVideoMode(const QByteArray &getParameterBody)
{
    const WfdVideoMode fallback = defaultWfdVideoMode();
    const QByteArray value = formatsValue(getParameterBody);
    if (value.isEmpty() || value == "none")
        return fallback;

    const QList<QByteArray> tokens = value.split(' ');
    QList<QByteArray> parts;
    parts.reserve(tokens.size());
    for (const QByteArray &token : tokens) {
        if (!token.isEmpty())
            parts.append(token);
    }
    // native pref profile level cea vesa hh …
    if (parts.size() < 7)
        return fallback;

    quint32 cea = 0;
    quint32 vesa = 0;
    quint32 hh = 0;
    if (!parseHex32(parts.at(4), &cea) || !parseHex32(parts.at(5), &vesa)
        || !parseHex32(parts.at(6), &hh)) {
        qWarning() << "Could not parse wfd_video_formats bitmaps" << value;
        return fallback;
    }

    int maxWidth = 0;
    int maxHeight = 0;
    if (parts.size() >= 13) {
        parseDec(parts.at(11), &maxWidth);
        parseDec(parts.at(12), &maxHeight);
    }

    QList<Candidate> sinkModes;
    collect(&sinkModes, WfdVideoMode::Table::Cea, kCea, cea, maxWidth, maxHeight, true);
    collect(&sinkModes, WfdVideoMode::Table::Vesa, kVesa, vesa, maxWidth, maxHeight, true);
    collect(&sinkModes, WfdVideoMode::Table::Hh, kHh, hh, maxWidth, maxHeight, true);
    if (sinkModes.isEmpty()) {
        collect(&sinkModes, WfdVideoMode::Table::Cea, kCea, cea, maxWidth, maxHeight, false);
        collect(&sinkModes, WfdVideoMode::Table::Vesa, kVesa, vesa, maxWidth, maxHeight, false);
        collect(&sinkModes, WfdVideoMode::Table::Hh, kHh, hh, maxWidth, maxHeight, false);
    }
    if (sinkModes.isEmpty()) {
        qWarning() << "Sink wfd_video_formats has no known modes" << value;
        return fallback;
    }

    const quint32 srcCea = sourceMask(kCea);
    const quint32 srcVesa = sourceMask(kVesa);
    const quint32 srcHh = sourceMask(kHh);
    QList<Candidate> intersection;
    for (const Candidate &c : sinkModes) {
        const quint32 src = (c.table == WfdVideoMode::Table::Cea) ? srcCea
            : (c.table == WfdVideoMode::Table::Vesa)               ? srcVesa
                                                                  : srcHh;
        if (src & (quint32(1) << c.res->bit))
            intersection.append(c);
    }
    const QList<Candidate> &pool = intersection.isEmpty() ? sinkModes : intersection;

    Candidate best = pool.first();
    for (int i = 1; i < pool.size(); ++i) {
        if (betterThan(pool.at(i), best))
            best = pool.at(i);
    }

    const WfdVideoMode mode = fromCandidate(best);
    qInfo() << "Selected WFD video mode" << mode.description() << "from" << value;
    return mode;
}
