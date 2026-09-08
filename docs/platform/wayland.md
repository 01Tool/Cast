# Wayland / Treeland capture

A Wayland client cannot screenshot the desktop by itself. Equal-quality mirroring on DDE Wayland depends on the compositor and portal, not on DTK.

## Required path

```
xdg-desktop-portal ScreenCast → PipeWire → encoder → WFD
```

The app requests a ScreenCast session. The portal backend asks the compositor. Frames arrive as a PipeWire stream. The cast engine feeds that stream into the same encode path used on X11 (WFD RTP or DLNA HTTP).

This is the same approach GNOME Network Displays uses when the mutter (or other) screencast portal is available.

## DDE status (deepin V25)

Treeland is DDE’s wlroots-based Wayland compositor. **Cast does not speak Treeland protocols.** It uses `org.freedesktop.portal.ScreenCast`; `xdg-desktop-portal-dde` talks to Treeland (`treeland_capture_*`, `zwlr_screencopy`, PipeWire) behind that portal.

On V25, `/usr/share/xdg-desktop-portal/portals/dde.portal` lists `org.freedesktop.impl.portal.ScreenCast`. `PortalCapture` creates a ScreenCast session, lets the user pick a monitor, and opens the PipeWire remote. `GstEncoder` then runs `pipewiresrc` (package `gstreamer1.0-pipewire`). File cast still skips capture.

An **X11** session can export the ScreenCast interface with `AvailableSourceTypes = 0`. That is not Treeland support. Never fall back to X11 grab while Wayland is active.

Talking to `treeland-capture` from `ot-cast` stays a last resort.

## Invalid fallback

Grabbing via X11 APIs while `WAYLAND_DISPLAY` is set only sees **XWayland** windows. Native DDE / Qt Wayland surfaces are missing. Do not treat that as Wayland support. The engine should fail with a clear “screen capture is unavailable on this session” error.

## What the app does

1. Detect Wayland with `DGuiApplicationHelper::IsWaylandPlatform`.
2. `PortalCapture`: `CreateSession` → `SelectSources` (monitor + embedded cursor) → `Start` (picker) → `OpenPipeWireRemote`.
3. Copy the PipeWire fd and node onto `DisplaySource`. `GstEncoder` uses `pipewiresrc fd=3 path=<node>`, never `ximagesrc` / `x11grab`.
4. If `AvailableSourceTypes` is 0, the user cancels, or `pipewiresrc` is missing, fail with a clear error. File → DLNA/Miracast still works.

WFD LPCM on this path is video-only (the GStreamer encode path muxes AAC). AAC + Pulse/PipeWire monitor still applies when the sink lists AAC.

## Desktop-environment work (outside this app)

A working Treeland session still needs the portal backend, PipeWire, and `gstreamer1.0-pipewire`. Confirm frames on a **Treeland** login; an X11 session is not that test.

## Treeland smoke (2026-09-08 / 2026-09-09) — not a pass

Session: `XDG_SESSION_TYPE=wayland`, `WAYLAND_DISPLAY=treeland.socket`, DDE. Monitor **DP-1** logical 1920×1080, `devicePixelRatio` 2, native 3840×2160. Qt lists it as “Unknown Unknown”. Packages: `xdg-desktop-portal` 1.22.1+ds-1deepin1, `xdg-desktop-portal-dde` 1.1.9, `gstreamer1.0-pipewire` 1.6.4-1deepin19 (`pipewiresrc` loads), PipeWire 1.6.4.

Portal (session bus `org.freedesktop.portal.Desktop`): `AvailableSourceTypes=3` (monitor+window), `AvailableCursorModes=0`, `version=1`. Impl (`org.freedesktop.impl.portal.desktop.dde`): source types 3, cursor modes **7**. Front portal v1 does not advertise cursor modes; `SelectSources` option is `cursor_mode` (not `cursor`). Sending an unadvertised mode **closes the session**.

Connect → picker **does** appear (`ScreenCastChooser`, DP-1). Allow is accepted. After Allow, Cast stays on **Select a screen to share…**, the window stops taking clicks, and Wi-Fi Direct never starts. Xiaomi Pad 7S Pro 12.5 (`36:E5:24:A6:07:5E`, WFD IEs yes) was in the list; **no** connect popup on the Pad. File cast was not used.

Dock then showed the **system** screen-share control (not `ot-cast-tray`; that plugin’s tip is “正在投屏”). Tooltip: **正在共享屏幕至[]** — sharing to an empty name. That is `xdg-desktop-portal-dde` / Treeland’s indicator after Allow. Empty `[]` is likely a missing portal app id (Cast was started as `./build/ot-cast`, not from `com.01tool.cast.desktop`). Capture on the compositor side may already be live; Cast never continued to P2P / `pipewiresrc`. This is **not** “Treeland cannot capture.”

What blocked the GUI (for the next attempt; **not** in tree):

1. `PortalCapture::start` used `QDBus::Block` for `Start`. `xdg-desktop-portal-dde` 1.1.9 impl `Start` is `oossa{sv} → ua{sv}` and may not return until Allow, so the Cast GUI thread is stuck for the whole picker.
2. `Request.Response` results include `streams` as `a(ua{sv})` with `size (ii)`. A Qt slot `onResponse(uint, QVariantMap)` demarshals that on the GUI thread: flood of `QDBusArgument: write from a read-only object` (200k+ lines in `~/.cache/01tool/ot-cast/ot-cast.log`) and the handler never returns.
3. Calling `OpenPipeWireRemote` (blocking) from inside that `Response` handler deadlocks the portal (it cannot reply until the signal handler returns).
4. A slot typed `QDBusArgument` does not match `a{sv}` (`bus.connect` returns false → **Could not listen for the ScreenCast portal reply.**). A `QDBusMessage`-only slot does not match `Response(u, a{sv})` either. Connecting with signature `ua{sv}` forces the same QVariantMap demarshal.
5. Round-tripping `wpa_supplicant` `P2PDeviceConfig` (nested DeviceType struct) aborted with libdbus `type struct 114 not a basic type`.

Do **not** X11-grab as a workaround. Next cut should: never `QDBus::Block` `Start` on the GUI thread; parse `streams` without mapping the whole dict to `QVariantMap`; `OpenPipeWireRemote` only after returning from `Response` (`asyncCall`). Omit `cursor_mode` when `AvailableCursorModes` is 0. Do not round-trip `P2PDeviceConfig`. Confirm with this Pad: picker → Allow → P2P prompt → `pipewiresrc` in the log → picture. WFD LPCM on this path stays video-only until the GStreamer encode path muxes it.
