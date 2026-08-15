# Cast (`ot-cast`)

A 01tool DTK app that casts the local screen to a wireless display. **Miracast** (Wi-Fi Display) runs on X11, with a Wayland capture stub. **DLNA** Digital Media Renderer on the same LAN is the fallback for TVs that do not implement WFD well. The UI names the protocol; DLNA is not Miracast. See [docs/protocols/README.md](docs/protocols/README.md).

## Verdict

A DTK app can provide this feature. DTK only covers the UI. Miracast (Wi-Fi Direct, WFD/RTSP, RTP) and DLNA (SSDP, HTTP, AVTransport) are engine work.

| Goal | Feasible? |
|------|-----------|
| DTK app that looks native on DDE | Yes |
| Mirror to many Miracast TVs/dongles on **X11** | Yes, with hardware caveats |
| Reach TVs that only do **DLNA** well | Yes, as a labeled same-LAN backend (live MPEG-TS) |
| Same capture on **Wayland / Treeland** | Only after ScreenCast (or equivalent) exists |
| One binary, both sessions, degrade gracefully | Yes — that should be the design |
| Windows-quality “it just works” on every sink | No, not with current Linux WFD |

**Bottom line:** the DTK app is the easy part. X11 Miracast works when WFD is reused and chipset limits are accepted. DLNA is the fallback when P2P/WFD is immature. Wayland capture is a desktop-environment dependency.

Recommended first cut: DTK shell + X11 capture + NetworkManager P2P discovery, with a Wayland capture backend stubbed until the portal is ready.

## Build

```bash
sudo apt install \
  build-essential cmake pkg-config \
  qt6-base-dev qt6-tools-dev qt6-l10n-tools \
  libdtk6core-dev libdtk6gui-dev libdtk6widget-dev

cmake -S . -B build
cmake --build build
./build/ot-cast
```

Current cut: DTK window (Simplified and Traditional Chinese translations), NetworkManager P2P scan **and connect** (WPS PIN or confirm-on-TV pairing), WFD RTSP on port 7236, **and** SSDP MediaRenderer discovery with HTTP MPEG-TS + AVTransport Play (labeled **DLNA**). X11 grab of the **selected monitor** → H.264 (optional AAC-LC from the Pulse/PipeWire default-sink monitor). Miracast scales to the sink’s WFD video mode and sends MPEG-TS/RTP. DLNA caps at 1280×720@30 and serves TS over HTTP.

Runtime extras:

```bash
sudo apt install gstreamer1.0-tools pulseaudio-utils
```

`pulseaudio-utils` provides `pactl` so the encoder can find the default-sink monitor. PipeWire users need `pipewire-pulse`.

On this deepin image, `gstreamer1.0-plugins-bad` 1.24.6 ships `mpegtsmux`/`h264parse` built as 1.26, so GStreamer 1.24 will not load them. The encoder then uses `ffmpeg -f x11grab … -f rtp_mpegts`. A firewall must allow TCP 7236 and UDP RTP toward the sink.

## Package

On a Deepin / DTK6 system, build both a `.deb` and an AppImage:

```bash
sudo apt build-dep .
./scripts/package.sh
```

Only one format:

```bash
./scripts/package.sh --deb
./scripts/package.sh --appimage
```

Artifacts land in `dist/`. `scripts/package.sh --help` lists jobs, output dir, and clean flags.

Debian packaging lives in `debian/` (debhelper + CMake, DTK6). You can still call `dpkg-buildpackage -us -uc -b` by hand. That installs the binary to `/usr/bin/ot-cast`, translations to `/usr/share/ot-cast/translations`, and the desktop file plus SVG icon into the applications menu.

The AppImage bundles Qt 6 and DTK6. It still needs host NetworkManager, `ffmpeg` or `gst-launch-1.0`, and `pactl`.

## Release

Push a tag that matches `CMakeLists.txt` and `debian/changelog` (for example `v0.1.0`). The GitHub **Release** workflow runs the protocol checks and publishes a source tarball on the tag. Build the `.deb` on Deepin as above; Ubuntu runners do not ship DTK6.

## Documents

| Document | Path |
|----------|------|
| Rules for agents (commits, references, architecture constraints) | [AGENTS.md](AGENTS.md) |
| Can the feature be built, and what DTK does vs does not cover | [docs/feasibility.md](docs/feasibility.md) |
| Recommended layers and first implementation cut | [docs/architecture.md](docs/architecture.md) |
| X11 screen capture | [docs/platform/x11.md](docs/platform/x11.md) |
| Wayland / Treeland screen capture | [docs/platform/wayland.md](docs/platform/wayland.md) |
| Hardware, P2P vs DLNA, sink, latency, and audio limits | [docs/constraints.md](docs/constraints.md) |
| Miracast vs DLNA transports | [docs/protocols/README.md](docs/protocols/README.md) |
| Miracast / Wi-Fi Display | [docs/protocols/miracast.md](docs/protocols/miracast.md) |
| DLNA / UPnP AV (planned) | [docs/protocols/dlna.md](docs/protocols/dlna.md) |
| Existing projects and Deepin pieces to reuse | [docs/references.md](docs/references.md) |

## Existing Deepin pieces

Deepin already has related work. A new app should start from these rather than rewriting WFD from scratch:

- [`linuxdeepin/deepin-network-displays`](https://github.com/linuxdeepin/deepin-network-displays) — fork of GNOME Network Displays
- **无线投屏** quick-panel plugin in [`dde-tray-loader`](https://github.com/linuxdeepin/dde-tray-loader) (`wireless-casting`)
