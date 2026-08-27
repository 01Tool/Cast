#pragma once

#include <QByteArray>
#include <QString>

// One CEA / VESA / HH resolution from Wi-Fi Display wfd_video_formats.
struct WfdVideoMode {
    enum class Table { Cea, Vesa, Hh };

    int width = 1280;
    int height = 720;
    int fps = 30;
    Table table = Table::Cea;
    int bit = 5;

    bool isValid() const;
    QString description() const;
    // Single-mode value for SET_PARAMETER (one bit set).
    QByteArray formatsParameter() const;
};

// 1280x720p30 (CEA bit 5). Used when the sink omits usable formats.
WfdVideoMode defaultWfdVideoMode();

// Source-supported bitmap for GET_PARAMETER replies (progressive modes we can encode).
QByteArray wfdSourceFormatsParameter();

// Parse a GET_PARAMETER body (or a bare wfd_video_formats value) and pick one mode.
// Prefers progressive, then ≤30 fps, then the closest aspect ratio to the captured
// screen (when sourceWidth/Height are set), then the largest resolution the sink listed.
WfdVideoMode selectWfdVideoMode(const QByteArray &getParameterBody, int sourceWidth = 0,
                                int sourceHeight = 0);
