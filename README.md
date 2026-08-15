# Miracast DTK App

A DTK desktop app that mirrors the local screen to Miracast (Wi-Fi Display) sinks, on both X11 and Wayland.

This folder currently holds the feasibility and architecture notes. Implementation has not started.

## Verdict

A DTK app can provide this feature. DTK only covers the UI. Miracast itself is a system-level stack (Wi-Fi Direct, WFD/RTSP, capture, encode, RTP).

| Goal | Feasible? |
|------|-----------|
| DTK app that looks native on DDE | Yes |
| Mirror to many Miracast TVs/dongles on **X11** | Yes, with hardware caveats |
| Same feature on **Wayland / Treeland** | Only after ScreenCast (or equivalent) exists |
| One binary, both sessions, degrade gracefully | Yes — that should be the design |
| Windows-quality “it just works” on every sink | No, not with current Linux WFD |

**Bottom line:** the DTK app is the easy part. The feature is satisfied on X11 if an existing WFD stack is reused and chipset/sink limits are accepted. Wayland support is a desktop-environment dependency, not a DTK one.

Recommended first cut: DTK shell + X11 capture + NetworkManager P2P discovery, with a Wayland capture backend stubbed until the portal is ready.

## Build

```bash
sudo apt install \
  build-essential cmake pkg-config \
  qt6-base-dev qt6-tools-dev qt6-l10n-tools \
  libdtk6core-dev libdtk6gui-dev libdtk6widget-dev

cmake -S . -B build
cmake --build build
./build/deepin-miracast
```

Current cut: DTK window (Simplified and Traditional Chinese translations), NetworkManager P2P scan **and connect** (WPS PIN or confirm-on-TV pairing), WFD RTSP on port 7236, and X11 grab of the **selected monitor** → scale to the sink’s WFD video mode → `x264enc` → MPEG-TS/RTP. Optional system audio (AAC-LC from the Pulse/PipeWire default-sink monitor) when the sink advertises AAC.

Runtime extras:

```bash
sudo apt install gstreamer1.0-tools pulseaudio-utils
```

`pulseaudio-utils` provides `pactl` so the encoder can find the default-sink monitor. PipeWire users need `pipewire-pulse`.

On this deepin image, `gstreamer1.0-plugins-bad` 1.24.6 ships `mpegtsmux`/`h264parse` built as 1.26, so GStreamer 1.24 will not load them. The encoder then uses `ffmpeg -f x11grab … -f rtp_mpegts`. A firewall must allow TCP 7236 and UDP RTP toward the sink.

## Documents

| Document | Path |
|----------|------|
| Rules for agents (commits, references, architecture constraints) | [AGENTS.md](AGENTS.md) |
| Can the feature be built, and what DTK does vs does not cover | [docs/feasibility.md](docs/feasibility.md) |
| Recommended layers and first implementation cut | [docs/architecture.md](docs/architecture.md) |
| X11 screen capture | [docs/platform/x11.md](docs/platform/x11.md) |
| Wayland / Treeland screen capture | [docs/platform/wayland.md](docs/platform/wayland.md) |
| Hardware, P2P, sink, latency, and audio limits | [docs/constraints.md](docs/constraints.md) |
| Existing projects and Deepin pieces to reuse | [docs/references.md](docs/references.md) |

## Existing Deepin pieces

Deepin already has related work. A new app should start from these rather than rewriting WFD from scratch:

- [`linuxdeepin/deepin-network-displays`](https://github.com/linuxdeepin/deepin-network-displays) — fork of GNOME Network Displays
- **无线投屏** quick-panel plugin in [`dde-tray-loader`](https://github.com/linuxdeepin/dde-tray-loader) (`wireless-casting`)
