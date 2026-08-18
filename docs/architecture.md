# Architecture

One DTK process, a display-server-agnostic cast engine, two capture backends, and **two labeled transports**. Widgets never call X11, Wayland, NetworkManager, or UPnP APIs directly.

Miracast (Wi-Fi Direct + WFD) is the first transport. Many TVs and Linux chipsets implement it poorly, so DLNA Digital Media Renderer on the same LAN is the same-LAN fallback. The device list must show the protocol. Do not present a DMR as a Miracast sink. See [protocols/README.md](protocols/README.md).

## Layers

```
DTK UI  (DApplication + DMainWindow)
    │  scan / connect / status / errors  (protocol visible on each row)
    ▼
CastEngine (Qt, display-server agnostic)
    ├── Discovery:
    │     P2P  → NetworkManager + wpa_supplicant WFD IEs
    │     DLNA → SSDP MediaRenderer
    ├── Session:
    │     WFD  → RTSP :7236 + RTP
    │     DLNA → HTTP stream + AVTransport Play
    └── Capture:
          X11     → ximagesrc / XShm
          Wayland → portal + PipeWire  (requires ScreenCast backend)
```

Detect the session with `DGuiApplicationHelper::IsXWindowPlatform` / `IsWaylandPlatform`, then select the capture backend.

## UI (DTK)

Responsibilities:

- Native DDE window (`DApplication`, `DMainWindow`, title bar, icons)
- Device list and refresh, with a **Miracast** or **DLNA** mark on each row
- Connect / disconnect, pairing prompts (WPS PIN or confirm-on-TV for P2P only), error dialogs
- Session status (searching, connecting, mirroring, failed)
- Optional: remember last sink (DConfig)
- Choose which monitor to mirror
- Toggle system audio (AAC when the sink supports it)

The UI talks only to `CastEngine` signals and slots (or an equivalent Qt interface). It does not open NetworkManager, GStreamer, portal, or UPnP connections itself.

Suggested CMake baseline (DTK6): `Qt6::Core`, `Qt6::Widgets`, `Dtk6::Core`, `Dtk6::Gui`, `Dtk6::Widget`. Add GStreamer / libnm / portal packages when the engine is wired.

## CastEngine

A Qt object that owns the session state machine:

1. **Idle** — Wi-Fi / P2P / LAN capability check
2. **Scanning** — P2P peers with WFD IEs **and** SSDP MediaRenderers
3. **Connecting** — P2P group + RTSP, **or** HTTP + AVTransport
4. **Streaming** — capture → encode → RTP **or** HTTP
5. **Failed / Stopped** — teardown; restore STA Wi-Fi after a P2P session

Subsystems:

| Subsystem | Role | Preferred implementation |
|-----------|------|--------------------------|
| Discovery (WFD) | List Miracast sinks | NetworkManager P2P + `wpa_supplicant` WFD IEs |
| Discovery (DLNA) | List MediaRenderers | Qt SSDP; see [protocols/dlna.md](protocols/dlna.md) |
| Pairing | WPS PIN / PBC | NM SecretAgent; P2P only |
| Session (WFD) | WFD RTSP handshake | GStreamer WFD bits from GNOME / deepin-network-displays |
| Session (DLNA) | HTTP + `SetAVTransportURI` | `DlnaSession` |
| Sink identity | Protocol on every row | `SinkDevice::protocol` is `Miracast` or `Dlna` |
| Capture | Frames + optional system audio | Backend interface + Pulse/PipeWire monitor |
| Encode | H.264 + AAC-LC | GStreamer (`x264enc` / `avenc_aac`) or ffmpeg |
| Transport | RTP or HTTP | Same encoder, different mux/send path |

Do not start from MiracleCast for a desktop app. It often requires stopping NetworkManager / `wpa_supplicant` and has a poor UX fit.

## Capture backend interface

```
CaptureBackend
  start(DisplaySource) -> frames
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
5. Hardware-encoder tuning after video, optional AAC, and a monitor picker work.
6. DLNA: SSDP list + HTTP live MPEG-TS to MediaRenderers, tagged in the UI. Use this when P2P/WFD is missing or unstable. See [protocols/dlna.md](protocols/dlna.md).
7. Device matrix: record which TVs accept live TS vs file-only, separately from WFD. See [devices.md](devices.md).

Items 1–6 are in the tree. Item 7 is filled from measured sessions, not from logos. Wayland still waits on ScreenCast.

## What not to put in widgets

- `XOpenDisplay`, `XShmGetImage`, GStreamer `ximagesrc` setup
- `xdg-desktop-portal` D-Bus calls
- NetworkManager / `wpa_supplicant` D-Bus
- SSDP / UPnP SOAP / local HTTP bind
- Encoder bitrate / RTP socket details

Those belong in `CastEngine` and the capture backends so X11 and Wayland stay swappable.
