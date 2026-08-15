# Platform capture

Screen capture is the only layer that must branch on the display server. Encode is shared. Discovery and send are **not**: Miracast uses P2P + RTP; DLNA uses SSDP + HTTP. See [../protocols/README.md](../protocols/README.md).

| Session | Document | First cut |
|---------|----------|-----------|
| X11 | [x11.md](x11.md) | Implement. `ximagesrc` / XShm. |
| Wayland / Treeland | [wayland.md](wayland.md) | Stub until ScreenCast (or equivalent) exists. |

Do not use X11 grab on a Wayland session. That only captures XWayland windows.

Detect the session with `DGuiApplicationHelper::IsXWindowPlatform` / `IsWaylandPlatform` in `CastEngine`, not in widgets.
