# Architecture

One DTK process, a display-server-agnostic cast engine, and two capture backends. Widgets never call X11 or Wayland capture APIs directly.

## Layers

```
DTK UI  (DApplication + DMainWindow)
    │  scan / connect / status / errors
    ▼
CastEngine (Qt, display-server agnostic)
    ├── Discovery: NetworkManager P2P / wpa_supplicant WFD IEs
    ├── Session:   WFD RTSP (reuse existing GStreamer WFD bits)
    └── Capture:
          X11     → ximagesrc / XShm
          Wayland → portal + PipeWire  (requires ScreenCast backend)
```

Detect the session with `DGuiApplicationHelper::IsXWindowPlatform` / `IsWaylandPlatform`, then select the capture backend.

## UI (DTK)

Responsibilities:

- Native DDE window (`DApplication`, `DMainWindow`, title bar, icons)
- Device list and refresh
- Connect / disconnect, pairing prompts, error dialogs
- Session status (searching, connecting, mirroring, failed)
- Optional: choose monitor, toggle audio, remember last sink (DConfig)

The UI talks only to `CastEngine` signals and slots (or an equivalent Qt interface). It does not open NetworkManager, GStreamer, or portal connections itself.

Suggested CMake baseline (DTK6): `Qt6::Core`, `Qt6::Widgets`, `Dtk6::Core`, `Dtk6::Gui`, `Dtk6::Widget`. Add GStreamer / libnm / portal packages when the engine is wired.

## CastEngine

A Qt object that owns the session state machine:

1. **Idle** — Wi-Fi / P2P capability check
2. **Scanning** — P2P peers with WFD IEs
3. **Connecting** — P2P group + RTSP SETUP
4. **Streaming** — capture → encode → RTP
5. **Failed / Stopped** — teardown, restore Wi-Fi if needed

Subsystems:

| Subsystem | Role | Preferred implementation |
|-----------|------|--------------------------|
| Discovery | List Miracast sinks | NetworkManager P2P + `wpa_supplicant` WFD IEs |
| Session | WFD RTSP handshake | GStreamer WFD elements from GNOME / deepin-network-displays |
| Capture | Frames (+ audio later) | Backend interface, see below |
| Encode | H.264 (AAC later) | GStreamer (`x264enc` / hardware encoder) |
| Transport | RTP/UDP to sink | GStreamer RTSP/RTP pipeline |

Do not start from MiracleCast for a desktop app. It often requires stopping NetworkManager / `wpa_supplicant` and has a poor UX fit.

## Capture backend interface

```
CaptureBackend
  start(source) -> frames
  stop()
  lastError()
```

Implementations:

| Backend | Session | Status |
|---------|---------|--------|
| `X11Capture` | X11 | Implement first. See [platform/x11.md](platform/x11.md). |
| `PortalCapture` | Wayland (and optionally X11) | Stub until DDE ScreenCast exists. See [platform/wayland.md](platform/wayland.md). |

If Wayland is active and `PortalCapture` cannot create a session, the engine must fail with a clear error. It must **not** silently fall back to X11 grab (that only sees XWayland windows).

## First implementation cut

1. DTK shell: window, empty device list, connect/disconnect placeholders.
2. Discovery: NetworkManager P2P scan, populate the list.
3. X11 capture + GStreamer WFD send path (reuse deepin/GNOME network-displays where possible).
4. Wayland `PortalCapture` stub that reports “ScreenCast unavailable”.
5. Audio, multi-monitor picker, and hardware-encoder tuning after video-only X11 works.

This order ships a usable X11 product without blocking on Treeland portal work.

## What not to put in widgets

- `XOpenDisplay`, `XShmGetImage`, GStreamer `ximagesrc` setup
- `xdg-desktop-portal` D-Bus calls
- NetworkManager / `wpa_supplicant` D-Bus
- Encoder bitrate / RTP socket details

Those belong in `CastEngine` and the capture backends so X11 and Wayland stay swappable.
