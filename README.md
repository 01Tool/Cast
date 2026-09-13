# Cast (`ot-cast`)

A 01tool DTK app ([github.com/01Tool/Cast](https://github.com/01Tool/Cast)) that casts the local screen to a wireless display. Current tree: **0.3.0**. **Miracast** (Wi-Fi Display) on **X11** uses Wi-Fi Direct, or **MS-MICE** on the same LAN for Windows Connect / Android. **Treeland** (DDE compositor) is not generic Wayland: capture is `TreelandCapture` through `xdg-desktop-portal` ScreenCast, not Treeland protocols and not the blocking Wayland `PortalCapture` path. Generic Wayland still uses `PortalCapture`. Never X11 grab on either. **DLNA** Digital Media Renderer on the same LAN is the fallback for TVs that do not implement WFD well. The UI names the protocol; DLNA is not Miracast. See [docs/protocols/README.md](docs/protocols/README.md). Licensed [GPL-3.0-or-later](LICENSE).

## Verdict

A DTK app can provide this feature. DTK only covers the UI. Miracast (Wi-Fi Direct or MS-MICE, WFD/RTSP, RTP) and DLNA (SSDP, HTTP, AVTransport) are engine work.

| Goal | Feasible? |
|------|-----------|
| DTK app that looks native on DDE | Yes |
| Mirror to many Miracast TVs/dongles on **X11** | Yes, with hardware caveats |
| Reach TVs that only do **DLNA** well | Yes, as a labeled same-LAN backend (live MPEG-TS) |
| Same capture on **Treeland** | Via `TreelandCapture` + ScreenCast portal; Tmall DLNA picture + AAC measured 2026-09-13 |
| Same capture on **generic Wayland** | Via `PortalCapture` + ScreenCast portal |
| One binary, both sessions, degrade gracefully | Yes — that should be the design |
| Windows-quality “it just works” on every sink | No, not with current Linux WFD |

**Bottom line:** the DTK app is the easy part. X11 Miracast works when WFD is reused and chipset limits are accepted. Same-LAN Windows Connect uses MS-MICE, not WPS. DLNA is the fallback when P2P/WFD is immature. Wayland capture is a desktop-environment dependency.

The X11 first cut is in the tree (DTK shell, P2P + MS-MICE + DLNA, WFD send, live HTTP TS). Treeland and generic Wayland both use the ScreenCast portal and `pipewiresrc` (`gstreamer1.0-pipewire`), as separate backends. Treeland → Tmall MagicBox DLNA has a measured live-TS picture **and AAC** (2026-09-13).

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

Current cut: DTK window (Simplified and Traditional Chinese translations), NetworkManager P2P scan **and connect** (WPS PIN or confirm-on-TV pairing), **MS-MICE** for Windows Connect (mDNS `_display._tcp`, TCP 7250 `SOURCE_READY`, then the same WFD RTSP), **and** SSDP MediaRenderer discovery with HTTP MPEG-TS + AVTransport Play (labeled **DLNA**). The window can send **this screen** or a local video / photo / audio file (DLNA serves the file; Miracast transcodes it into WFD RTP). X11 grab of the **selected monitor** in **physical pixels** (HiDPI `devicePixelRatio`) → H.264 (optional AAC-LC from the Pulse/PipeWire default-sink monitor, or WFD LPCM when the sink has no AAC). Miracast picks a WFD mode that matches the monitor’s aspect ratio and **letterboxes** instead of stretching. DLNA caps at 1920×1080@30. A DDE quick-panel plugin (`libot-cast-tray.so`) scans and connects through the app over D-Bus; it does not talk to NetworkManager or GStreamer itself.

Runtime extras:

```bash
sudo apt install gstreamer1.0-tools pulseaudio-utils
```

`pulseaudio-utils` provides `pactl` so the encoder can find the default-sink monitor. PipeWire users need `pipewire-pulse`.

On this deepin image, `gstreamer1.0-plugins-bad` 1.24.6 ships `mpegtsmux`/`h264parse` built as 1.26, so GStreamer 1.24 will not load them. The encoder then uses `ffmpeg -f x11grab … -f rtp_mpegts`.

Firewall and local D-Bus risks are listed in [docs/constraints.md](docs/constraints.md) §7. Short version: allow **inbound TCP 7236** (WFD RTSP), **outbound TCP 7250** (MS-MICE), UDP RTP to the sink’s `client_port`, SSDP 1900, mDNS 5353, and inbound TCP to the ephemeral DLNA HTTP port. `com.ot01tool.Cast` methods are not callable from arbitrary same-user processes.

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

Keep these three equal to each other and to the git tag (`v0.3.0` for this cut):

- `CMakeLists.txt` `project(ot-cast VERSION …)`
- `src/main.cpp` `setApplicationVersion`
- `debian/changelog` top stanza

`0.3.0` sends this screen or a local video / photo / audio file. Miracast muxes LPCM when the sink has no AAC. DLNA live MPEG-TS stays up across the TV’s first probe GET. The sender is **X11** Miracast (P2P and MS-MICE) plus labeled DLNA, zh_CN/zh_TW. Treeland uses a separate ScreenCast backend; Tmall DLNA picture + AAC is measured. Do not claim every TV or low latency.

Push `main`, then a tag that matches those three:

```bash
git tag -s v0.3.0 -m "Cast 0.3.0"
git push origin main v0.3.0
```

The GitHub **Release** workflow runs the protocol checks and publishes a **source tarball** (`ot-cast-0.3.0.tar.gz`) plus **SHA256SUMS** on the tag. Ubuntu runners have no DTK6, so that workflow does **not** attach `.deb` or AppImage files and does **not** claim amd64, arm64, or loong64 binaries.

```bash
sha256sum -c SHA256SUMS
```

`debian/control` is `Architecture: any`. A `.deb` built on Deepin matches **that host** (`amd64`, `arm64`, or `loong64`). Do not list those architectures on the Release page until the matching packages are actually attached (and listed in `SHA256SUMS`).

## License

[GPL-3.0-or-later](LICENSE). Same terms in `debian/copyright`. Source: [github.com/01Tool/Cast](https://github.com/01Tool/Cast).

## Documents

| Document | Path |
|----------|------|
| Rules for agents (commits, references, architecture constraints) | [AGENTS.md](AGENTS.md) |
| Can the feature be built, and what DTK does vs does not cover | [docs/feasibility.md](docs/feasibility.md) |
| Recommended layers and first implementation cut | [docs/architecture.md](docs/architecture.md) |
| X11 screen capture | [docs/platform/x11.md](docs/platform/x11.md) |
| Treeland screen capture (DDE compositor) | [docs/platform/treeland.md](docs/platform/treeland.md) |
| Generic Wayland screen capture | [docs/platform/wayland.md](docs/platform/wayland.md) |
| Hardware, P2P vs DLNA, sink, latency, and audio limits | [docs/constraints.md](docs/constraints.md) |
| Measured TV / dongle matrix | [docs/devices.md](docs/devices.md) |
| Miracast vs DLNA transports | [docs/protocols/README.md](docs/protocols/README.md) |
| Miracast / Wi-Fi Display | [docs/protocols/miracast.md](docs/protocols/miracast.md) |
| DLNA / UPnP AV | [docs/protocols/dlna.md](docs/protocols/dlna.md) |
| Existing projects and Deepin pieces to reuse | [docs/references.md](docs/references.md) |

## Existing Deepin pieces

Deepin already has related work. A new app should start from these rather than rewriting WFD from scratch:

- [`linuxdeepin/deepin-network-displays`](https://github.com/linuxdeepin/deepin-network-displays) — fork of GNOME Network Displays
- **无线投屏** quick-panel plugin in [`dde-tray-loader`](https://github.com/linuxdeepin/dde-tray-loader) (`wireless-casting`)
