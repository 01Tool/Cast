# Feasibility

A DTK application can expose Miracast screen mirroring. The toolkit is sufficient for a native DDE UI. It is not sufficient for the protocol, capture, or transport work.

## What DTK covers

DTK (deepin Tool Kit) is a UI and system-integration framework for deepin / UOS / DDE. For this app it supplies:

- Application shell: `DApplication`, `DMainWindow`, title bar, theming
- Device list, dialogs, status, notifications
- DConfig / settings, logging (`DLogManager`), DBus helpers
- Platform detection via `DGuiApplicationHelper::IsXWindowPlatform` / `IsWaylandPlatform`
- Window chrome on X11 and Treeland (radius, blur, CSD)

DTK’s platform abstraction is for **window decoration**, not screen capture. `DPlatformHandle` / `DPlatformTheme` do not grab frames, talk Wi-Fi Direct, or speak WFD.

## What DTK does not cover

These layers must live under the UI, in a display-server-agnostic engine plus platform backends:

| Layer | Job |
|------|-----|
| Wi-Fi Direct / P2P | Find the sink, form a P2P group |
| WFD / RTSP | Capability exchange and session setup |
| Capture | Desktop frames and optional system audio |
| Encode / send | H.264 (and AAC), RTP/UDP to the sink |

## Stack split

| Layer | Job | DTK’s role |
|------|-----|------------|
| UI | Scan, pair, connect, status | Yes — this is the DTK app |
| Wi-Fi Direct / P2P | Find the sink, form a P2P group | No — NetworkManager + `wpa_supplicant` (`CONFIG_P2P`, `CONFIG_WIFI_DISPLAY`) |
| WFD / RTSP | Capability exchange, session | No — reuse GStreamer / existing WFD code |
| Capture | Frames + optional audio | No — X11 grab vs portal / PipeWire |
| Encode / send | H.264 (+ AAC), RTP/UDP | No — GStreamer / FFmpeg |

## Existing Deepin work

This is not a green field. Deepin already shipped pieces of the same feature:

- `linuxdeepin/deepin-network-displays` is a fork of GNOME Network Displays (experimental Wi-Fi Display sender).
- deepin 23 added a **无线投屏** entry in the quick panel. The tray assets live in `dde-tray-loader` as `wireless-casting`.
- The official release note describes searching the same network for Miracast-capable devices and casting the desktop.

A new DTK app should treat those as the starting point: reuse the WFD/P2P stack, replace or wrap the UI with DTK, and add an explicit X11 / Wayland capture split. A full protocol rewrite is usually the wrong first move.

## X11 vs Wayland in one sentence

- **X11:** the feature can work today with known Linux sender techniques.
- **Wayland / Treeland:** only if the compositor (via `xdg-desktop-portal` ScreenCast → PipeWire, or an equivalent capture protocol) can share the screen. Falling back to X11 grab inside a Wayland session is not Wayland support.

Details: [platform/x11.md](platform/x11.md), [platform/wayland.md](platform/wayland.md).

## Verdict

| Goal | Feasible? |
|------|-----------|
| DTK app that looks native on DDE | Yes |
| Mirror to many Miracast TVs/dongles on **X11** | Yes, with hardware caveats |
| Same feature on **Wayland / Treeland** | Only after ScreenCast (or equivalent) exists |
| One binary, both sessions, degrade gracefully | Yes — that should be the design |
| Windows-quality “it just works” on every sink | No, not with current Linux WFD |

The DTK app is the easy part. The feature is satisfied on X11 if an existing WFD stack is reused and chipset/sink limits are accepted. Wayland support is a **desktop-environment dependency**, not a DTK one.

See also: [constraints.md](constraints.md), [architecture.md](architecture.md).
