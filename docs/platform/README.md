# Platform capture

Screen capture is the only layer that must branch on the display server. Encode is shared. Discovery and send are **not**: Miracast uses Wi-Fi Direct **or** MS-MICE, then RTP; DLNA uses SSDP + HTTP. See [../protocols/README.md](../protocols/README.md).

| Session | Document | First cut |
|---------|----------|-----------|
| X11 | [x11.md](x11.md) | Implement. `ximagesrc` / XShm. |
| Treeland (DDE) | [treeland.md](treeland.md) | `TreelandCapture` via ScreenCast portal (async Start). Not Treeland protocols. |
| Generic Wayland | [wayland.md](wayland.md) | `PortalCapture` via `org.freedesktop.portal.ScreenCast` → PipeWire. |

Do not use X11 grab on a Wayland or Treeland session. That only captures XWayland windows.

Detect **Treeland first** (`WAYLAND_DISPLAY` / `DESKTOP_SESSION` contain `treeland`), then generic Wayland with `DGuiApplicationHelper::IsWaylandPlatform`, then X11 with `IsXWindowPlatform`. Do this in `CastEngine`, not in widgets. Treeland is not the Wayland method.
