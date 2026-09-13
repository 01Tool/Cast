# Treeland capture

Treeland is DDE’s wlroots-based compositor. It is **not** generic Wayland for this app.

DTK `IsWaylandPlatform` is true on a Treeland login because window chrome uses `DTreeLandPlatformWindowInterface` (`treeland_personalization`). That is decoration, not capture. Folding Treeland into `PortalCapture` / the Wayland method is what hung Cast after Allow on 2026-09-08.

## Detect

Check the compositor **before** `DGuiApplicationHelper::IsWaylandPlatform`:

1. `WAYLAND_DISPLAY` contains `treeland` (this host: `treeland.socket`), or
2. `DESKTOP_SESSION` / `XDG_SESSION_DESKTOP` contains `treeland`.

Then `CastEngine` sets `DisplayServer::Treeland` and `TreelandCapture`. The UI must say **Display server: Treeland**, not Wayland. Generic kwin/mutter sessions are [wayland.md](wayland.md) **not continued**: `PortalCapture` stays in the tree and is not selected.

Never X11-grab here. That only sees XWayland windows.

## Required path

Same portal as other Wayland desktops. **Cast does not speak Treeland protocols** (`treeland_capture_*`, `zwlr_screencopy`). `xdg-desktop-portal-dde` talks to Treeland behind `org.freedesktop.portal.ScreenCast`.

```
TreelandCapture → xdg-desktop-portal ScreenCast → PipeWire → encoder → WFD / DLNA
```

`TreelandCapture` is the experimental async cut of that portal:

1. `CreateSession` / `SelectSources` (omit `cursor_mode` when `AvailableCursorModes` is 0). Qt D-Bus is fine here: those replies have no `streams`.
2. `Start` with `asyncCall`. Do **not** `QDBus::Block` on the GUI thread; the dde impl may not return until Allow.
3. **Do not** `QDBusConnection::connect` `Request.Response` for Start. Qt 6 `QDBusMessagePrivate::fromDBusMessage` demarshals every argument (`u` + `a{sv}`) into `QVariant` before any slot runs. `streams` `a(ua{sv})` with `size (ii)` then floods `QDBusArgument: write from a read-only object` and the GUI dies after Allow. That signal is unicast to Qt’s unique name, so a second bus connection will not see it unless it is a monitor. `TreelandCapture` opens a private libdbus connection, `BecomeMonitor`s `Request.Response`, parses `streams` with `DBusMessageIter`, then `OpenPipeWireRemote` (`asyncCall`).
4. Emit `ready` with the PipeWire fd (and node if the watcher got one). `pipewiresrc` may omit `path` when the node is unknown: the portal remote only exposes the ScreenCast node. `CastEngine` waits for `ready` before DLNA HTTP or WFD.

`GstEncoder` then uses `pipewiresrc` (`gstreamer1.0-pipewire`) with `always-copy=true` and no `path=` so Treeland DMA-BUF can feed software `x264enc`. File cast still skips capture. DLNA AAC uses the Pulse default-sink monitor with `provide-clock=false`.

## Why not the Wayland method

Generic `PortalCapture` (blocking `Start` + `QVariantMap` `streams`) matches mutter-style portals. On Treeland + `xdg-desktop-portal-dde` 1.1.9 it freezes the GUI after Allow. Keep that class in the tree; generic Wayland is not continued. Do not share the blocking `start()` on this compositor.

## DDE status (deepin V25)

`/usr/share/xdg-desktop-portal/portals/dde.portal` lists `org.freedesktop.impl.portal.ScreenCast`. Front portal: `AvailableSourceTypes=3`, `AvailableCursorModes=0`, `version=1`. Impl cursor modes `7`. Unadvertised `cursor_mode` **closes the session**.

Talking to `treeland-capture` from `ot-cast` stays a last resort.

## Treeland smoke (2026-09-08 / 2026-09-09) — not a pass

Session: `XDG_SESSION_TYPE=wayland`, `WAYLAND_DISPLAY=treeland.socket`, DDE. Monitor **DP-1** logical 1920×1080, `devicePixelRatio` 2, native 3840×2160. Packages: `xdg-desktop-portal` 1.22.1+ds-1deepin1, `xdg-desktop-portal-dde` 1.1.9, `gstreamer1.0-pipewire` 1.6.4-1deepin19 (`pipewiresrc` loads), PipeWire 1.6.4.

Connect → picker **does** appear (`ScreenCastChooser`, DP-1). Allow is accepted. After Allow, Cast stayed on **Select a screen to share…**, the window stopped taking clicks, and Wi-Fi Direct never started. Xiaomi Pad 7S Pro 12.5 was in the list; **no** connect popup on the Pad. File cast was not used.

Dock then showed the **system** screen-share control (not `ot-cast-tray`; that plugin’s tip is “正在投屏”). Tooltip: **正在共享屏幕至[]** — sharing to an empty name. That is `xdg-desktop-portal-dde` / Treeland’s indicator after Allow. Empty `[]` is likely a missing portal app id (Cast was started as `./build/ot-cast`, not from `com.01tool.cast.desktop`). Capture on the compositor side may already be live; Cast never continued to P2P / `pipewiresrc`. This is **not** “Treeland cannot capture.”

What blocked the GUI (now in `TreelandCapture`, not `PortalCapture`):

1. `PortalCapture::start` used `QDBus::Block` for `Start`. `xdg-desktop-portal-dde` 1.1.9 impl `Start` is `oossa{sv} → ua{sv}` and may not return until Allow, so the Cast GUI thread is stuck for the whole picker.
2. `Request.Response` results include `streams` as `a(ua{sv})` with `size (ii)`. A Qt slot `onResponse(uint, QVariantMap)` demarshals that on the GUI thread: flood of `QDBusArgument: write from a read-only object` (200k+ lines in `~/.cache/01tool/ot-cast/ot-cast.log`) and the handler never returns.
3. Calling `OpenPipeWireRemote` (blocking) from inside that `Response` handler deadlocks the portal (it cannot reply until the signal handler returns).
4. A slot typed `QDBusArgument` does not match `a{sv}` (`bus.connect` returns false → **Could not listen for the ScreenCast portal reply.**). A `QDBusMessage`-only slot does not match `Response(u, a{sv})` either. Connecting with signature `ua{sv}` forces the same QVariantMap demarshal.
5. Round-tripping `wpa_supplicant` `P2PDeviceConfig` (nested DeviceType struct) aborted with libdbus `type struct 114 not a basic type`.

## Treeland smoke (2026-09-13) — DLNA pass, video + AAC

Same session type. Tmall MagicBox_M18 (`192.168.31.8`) labeled **DLNA**. Connect → picker → Allow DP-1 → `pipewiresrc` negotiates `RGBx` 3840×2160 `max-framerate=60/1`. Encoder scales to 1920×1080 H.264 main Annex-B, `mpegtsmux alignment=0`. First picture was **video-only**; AAC followed with Pulse default-sink `.monitor` and `pulsesrc provide-clock=false` (a second live clock had stalled the mux). User: desktop **and** system audio on the box. With AAC, `gst-launch` ~260% CPU, tens of MiB of TS.

Do **not** set `pipewiresrc path=<node>` on this portal remote: a stale id waits forever (0% CPU, ~65 B out, MagicBox GET `/cast.ts` every ~6 s with no picture). Do not `QDBusConnection::connect` Start `Response`. Do not fold this into `PortalCapture`.

Xiaomi P2P on Treeland is still unmeasured. WFD LPCM on this path stays video-only until the encode path muxes it.
