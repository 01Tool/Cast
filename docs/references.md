# References

Projects and docs that this design builds on. Prefer reuse over a new WFD implementation.

## Deepin

| Project | Role |
|---------|------|
| [linuxdeepin/deepin-network-displays](https://github.com/linuxdeepin/deepin-network-displays) | Deepin fork of GNOME Network Displays. Experimental Wi-Fi Display sender. Portal first, X11 `ximagesrc` fallback. |
| [linuxdeepin/dde-tray-loader](https://github.com/linuxdeepin/dde-tray-loader) | Ships the **无线投屏** (`wireless-casting`) quick-panel plugin and icons. |
| [linuxdeepin/xdg-desktop-portal-dde](https://github.com/linuxdeepin/xdg-desktop-portal-dde) | DDE portal backend. Confirm ScreenCast vs Screenshot on the target version. |
| [linuxdeepin/treeland](https://github.com/linuxdeepin/treeland) | DDE Wayland compositor (wlroots + QtQuick). Capture must go through it or a portal backend. |
| DTK development skill (`~/.agents/skills/deepin/dtk-development/SKILL.md`) | App CMake, `DApplication` / `DMainWindow`, platform detection, packaging. |

deepin 23 release notes introduced wireless screen casting in the quick panel and later fixed the plugin’s incomplete options display.

## Upstream Linux senders

| Project | Role |
|---------|------|
| [GNOME Network Displays](https://gitlab.gnome.org/GNOME/gnome-network-displays) | Closest production open-source sender. Miracast + Chromecast (not DLNA). PipeWire portal, X11 fallback, NetworkManager P2P. Still described as experimental. |
| [MiracleCast](https://github.com/albfan/miraclecast) | Low-level WFD toolkit. Poor desktop fit (often needs NM/wpa stopped). Do not use as the app base. |
| [FluxCast](https://github.com/IlyaP358/fluxcast) | Newer Python WFD client; native wlroots path, ~1 s latency reported. Useful as a protocol reference, not as the DTK UI. |

## System pieces

| Component | Why it matters |
|-----------|----------------|
| NetworkManager ≥ 1.15.2 | P2P together with `wpa_supplicant` (not `iwd`); SecretAgent for WPS PIN/PBC |
| `wpa_supplicant` | `CONFIG_P2P` + `CONFIG_WIFI_DISPLAY` |
| GStreamer + `gst-rtsp-server` | Capture, H.264, RTSP/RTP |
| PipeWire + WirePlumber | Wayland (and modern X11) audio/video capture |
| PulseAudio / `pipewire-pulse` | Default-sink `.monitor` for system audio |
| `xdg-desktop-portal` | ScreenCast session on Wayland |
| SSDP / UPnP AV / DLNA DMR | Same-LAN discovery (`MediaRenderer:1`) and `AVTransport` Play |
| GUPnP / GSSDP / gupnp-av | Optional C stack; first cut uses Qt Network instead |

## DLNA / UPnP

| Project | Role |
|---------|------|
| [UPnP AV Architecture:1](https://upnp.org/specs/av/UPnP-av-AVArchitecture-v1.pdf) | Control Point + MediaRenderer + MediaServer roles; `SetAVTransportURI` then `Play` |
| [AVTransport:2](https://www.upnp.org/specs/av/UPnP-av-AVTransport-v2-Service.pdf) | `SetAVTransportURI`, `Play`, `Stop`, `GetTransportInfo` |
| [ConnectionManager](https://upnp.org/specs/av/UPnP-av-ConnectionManager-v3-Service-20101231.pdf) | `GetProtocolInfo` so the HTTP MIME/profile is not a guess |
| [MediaRenderer:3](https://upnp.org/specs/av/UPnP-av-MediaRenderer-v3-Device-20101231.pdf) | Device template; first cut still searches `MediaRenderer:1` |
| [DLNA guidelines](https://www.dlna.org/) | Historical media profiles and HTTP headers (`contentFeatures.dlna.org`). The alliance has dissolved; treat headers as de facto. |
| [GNOME GUPnP](https://gitlab.gnome.org/GNOME/gupnp) | SOAP / device control-point stack |
| [GNOME GSSDP](https://gitlab.gnome.org/GNOME/gssdp) | SSDP M-SEARCH / notify |
| [GNOME gupnp-av](https://gitlab.gnome.org/GNOME/gupnp-av) | DIDL-Lite and ProtocolInfo helpers |
| [Rygel](https://gitlab.gnome.org/GNOME/rygel) | Linux DMS/DMR; profile/header reference, not this app’s UI |
| [link89/dlna-cast](https://github.com/link89/dlna-cast) | CLI proof: live desktop → HLS → `SetAVTransportURI`. Not the DTK UI. |

Live desktop over DMR is HTTP pull, not WFD RTP. See [protocols/dlna.md](protocols/dlna.md). GNOME Network Displays does not implement this path (it has Chromecast instead). Do not add Chromecast here.

## Capture notes

- GNOME Network Displays README: stream the selected monitor if the mutter screencast portal is available; otherwise fall back to X11 frame grabbing.
- Arch Wiki XDG Desktop Portal table (verify when targeting a DDE release): `xdg-desktop-portal-dde` Screenshot yes, ScreenCast historically no.
- X11 `ximagesrc` lives in `gstreamer1.0-plugins-good`. Missing that plugin is a common “fallback to X11 failed” cause.
- WFD `wfd_audio_codecs` AAC bit 0 is 48 kHz stereo; bit 1 is 44.1 kHz. This sender muxes AAC-LC into MPEG-TS and skips audio when the sink lists only LPCM.
- Multi-monitor: crop the selected `QScreen::geometry()`; do not grab the virtual union of all outputs.

## Related local docs

- [feasibility.md](feasibility.md)
- [architecture.md](architecture.md)
- [platform/x11.md](platform/x11.md)
- [platform/wayland.md](platform/wayland.md)
- [constraints.md](constraints.md)
- [protocols/README.md](protocols/README.md)
- [protocols/dlna.md](protocols/dlna.md)
- [protocols/miracast.md](protocols/miracast.md)

## Where used

Agents **must** append a row here for every document or repo they reference, and point at the exact local place that used it. See [AGENTS.md](../AGENTS.md).

| Date | Source | What was used | Used in |
|------|--------|---------------|---------|
| 2026-08-13 | [linuxdeepin/deepin-network-displays](https://github.com/linuxdeepin/deepin-network-displays) | Fork of GNOME Network Displays; portal-first sender with X11 fallback | [README.md](../README.md) Existing Deepin pieces; [feasibility.md](feasibility.md) Existing Deepin work; [architecture.md](architecture.md) Session / first cut; this file Deepin table |
| 2026-08-13 | [linuxdeepin/dde-tray-loader](https://github.com/linuxdeepin/dde-tray-loader) | `wireless-casting` quick-panel plugin and icons | [README.md](../README.md) Existing Deepin pieces; [feasibility.md](feasibility.md) Existing Deepin work; this file Deepin table |
| 2026-08-13 | [linuxdeepin/xdg-desktop-portal-dde](https://github.com/linuxdeepin/xdg-desktop-portal-dde) | DDE portal backend; Screenshot vs ScreenCast status | [platform/wayland.md](platform/wayland.md) DDE status; [constraints.md](constraints.md) permissions; this file Deepin table |
| 2026-08-13 | [linuxdeepin/treeland](https://github.com/linuxdeepin/treeland) | DDE Wayland compositor (wlroots + QtQuick) | [platform/wayland.md](platform/wayland.md) DDE status / DE work; this file Deepin table |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/SKILL.md` | DTK app CMake, `DApplication` / `DMainWindow`, platform detection | [feasibility.md](feasibility.md) What DTK covers; [architecture.md](architecture.md) UI / CMake; [platform/README.md](platform/README.md) session detect; this file Deepin table |
| 2026-08-13 | [GNOME Network Displays](https://gitlab.gnome.org/GNOME/gnome-network-displays) | WFD sender: portal + PipeWire, X11 `ximagesrc` fallback, NM P2P | [architecture.md](architecture.md) Session table; [platform/x11.md](platform/x11.md) Approach; [platform/wayland.md](platform/wayland.md) Required path; this file Upstream table |
| 2026-08-13 | [MiracleCast](https://github.com/albfan/miraclecast) | Low-level WFD toolkit; NM/wpa often must be stopped | [constraints.md](constraints.md) §6 Wrong base; [architecture.md](architecture.md) do not start from MiracleCast; this file Upstream table |
| 2026-08-13 | [FluxCast](https://github.com/IlyaP358/fluxcast) | Newer WFD client; wlroots path; ~1 s latency | [constraints.md](constraints.md) latency; this file Upstream table |
| 2026-08-13 | [deepin 23 release notes](https://www.deepin.org/en/deepin-23-is-officially-released/) | Quick-panel wireless casting; Miracast device search | [README.md](../README.md) Existing Deepin pieces; [feasibility.md](feasibility.md) Existing Deepin work; this file Deepin notes |
| 2026-08-13 | [Arch Wiki: XDG Desktop Portal](https://wiki.archlinux.org/title/XDG_Desktop_Portal) | `xdg-desktop-portal-dde` Screenshot yes, ScreenCast historically no | [platform/wayland.md](platform/wayland.md) DDE status; this file Capture notes |
| 2026-08-13 | `docs/architecture.md` | Layers, CastEngine states, capture backends, first cut | `src/engine/castengine.*`; `src/capture/*`; `src/ui/mainwindow.*`; `CMakeLists.txt` |
| 2026-08-13 | `docs/platform/wayland.md` | No X11 grab on Wayland; ScreenCast missing error | `src/capture/portalcapture.cpp` `PortalCapture::start`; `src/engine/castengine.cpp` `selectCaptureBackend` |
| 2026-08-13 | `docs/platform/x11.md` | X11 backend first; `ximagesrc` later | `src/capture/x11capture.cpp` `X11Capture::start` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/app-dev-with-dtk.md` | DTK6 CMake, `DApplication`, debian-style deps | `CMakeLists.txt`; `src/main.cpp`; [README.md](../README.md) Build |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/application.md` | `DApplication`, single instance, product info, `loadTranslator` | `src/main.cpp` `main` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/window.md` | `DMainWindow`, `DTitlebar` | `src/ui/mainwindow.cpp` `MainWindow` / `setupUi` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/view.md` | `DListView` + `DStyledItemDelegate` | `src/ui/mainwindow.cpp` `setupUi` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/item-delegate.md` | `RoundedBackground`, `DStandardItem`, `DViewItemAction` | `src/ui/mainwindow.cpp` `setupUi` / `refreshSinkList` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/button.md` | `DSuggestButton` primary, `DPushButton` secondary | `src/ui/mainwindow.cpp` `setupUi` Connect/Scan/Disconnect |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/label.md` | `DLabel` + `DPalette` + `DFontSizeManager` | `src/ui/mainwindow.cpp` `setupUi` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/widgets/message.md` | `DMessageManager::sendMessage` | `src/ui/mainwindow.cpp` `onError` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/utilities/log.md` | `DLogManager::registerConsoleAppender` / `registerFileAppender` | `src/main.cpp` `main` |
| 2026-08-13 | `~/.agents/skills/deepin-skills/dtk-development/references/platform-abstraction.md` | `IsXWindowPlatform` / `IsWaylandPlatform` | `src/engine/castengine.cpp` `selectCaptureBackend` |
| 2026-08-13 | [NM Device.WifiP2P](https://networkmanager.dev/docs/api/1.44.4/gdbus-org.freedesktop.NetworkManager.Device.WifiP2P.html) | `StartFind`/`StopFind`, `PeerAdded`/`PeerRemoved`, `Peers`, `timeout` 1–600s | `src/discovery/p2pdiscovery.cpp` `startScan` / `onPeerAdded` / `loadExistingPeers` |
| 2026-08-13 | [NM WifiP2PPeer](https://networkmanager.dev/docs/api/1.52.0/gdbus-org.freedesktop.NetworkManager.WifiP2PPeer.html) | `Name`, `HwAddress`, `WfdIEs` | `src/discovery/p2pdiscovery.cpp` `readPeer` |
| 2026-08-13 | [GNOME Network Displays README](https://github.com/benzea/gnome-network-displays) | Ignore/flag peers with empty WFDIEs; source WFD IE `00000600901c4400c8` | `src/discovery/p2pdiscovery.cpp` `tryAdvertiseWfdIes` / `readPeer`; `src/engine/castengine.cpp` `onScanFinished` |
| 2026-08-13 | `docs/architecture.md` Discovery | NM P2P + WFD IEs; no MiracleCast; UI must not call NM | `src/discovery/p2pdiscovery.*`; `src/engine/castengine.cpp` `startScan` |
| 2026-08-13 | `docs/constraints.md` §1–2 | wpa_supplicant P2P/WFD; not iwd; true P2P not same-LAN | `src/discovery/p2pdiscovery.cpp` `startScan` adapter / Wi-Fi checks |
| 2026-08-13 | `/tmp/miracast-ref/gnome-network-displays/src/nd-wfd-p2p-sink.c` | `AddAndActivateConnection2`, volatile P2P profile, WFD IEs, ipv4 never-default | `src/session/p2psession.cpp` `activate` |
| 2026-08-13 | `/tmp/miracast-ref/gnome-network-displays/src/wfd/wfd-client.c` | M1–M5 OPTIONS / GET_PARAMETER / SET_PARAMETER / trigger SETUP; source as RTSP server :7236 | `src/session/wfdserver.cpp` `WfdSession` |
| 2026-08-13 | `/tmp/miracast-ref/gnome-network-displays/src/wfd/wfd-media-factory.c` | `ximagesrc` / `x264enc` / `mpegtsmux` video-only path | `src/session/gstencoder.cpp` `startGst` |
| 2026-08-13 | local `gst-inspect-1.0` on gstreamer 1.24.6 | `mpegtsmux`/`h264parse` from plugins-bad fail: plugin 1.26 vs gst 1.24 | `src/session/gstencoder.cpp` `start` ffmpeg fallback |
| 2026-08-13 | `ffmpeg -muxers` | `rtp_mpegts` MPEG-TS-over-RTP | `src/session/gstencoder.cpp` `startFfmpeg` |
| 2026-08-13 | `/tmp/miracast-ref/gnome-network-displays/src/app/nd-window.c` | X11 fallback `ximagesrc` when portal missing | `src/capture/x11capture.cpp` `start`; `src/session/gstencoder.cpp` |
| 2026-08-13 | `docs/platform/x11.md` | ximagesrc → encode → RTP | `src/session/gstencoder.cpp` `buildPipeline` |
| 2026-08-13 | `docs/architecture.md` first cut §3 | X11 capture + GStreamer WFD send | `src/session/*`; `src/engine/castengine.cpp` `connectToSink` |
| 2026-08-15 | `docs/platform/x11.md` Approach | Scale to sink-negotiated size after `ximagesrc` | `src/session/wfdvideomode.cpp` `selectWfdVideoMode`; `src/session/wfdserver.cpp` `parseSinkParams` / `sendSetParameter`; `src/session/gstencoder.cpp` `startGst` / `startFfmpeg` |
| 2026-08-15 | `docs/architecture.md` first cut §3 | X11 + GStreamer WFD send; widgets stay off the pipeline | `src/session/wfdvideomode.*`; `src/engine/castengine.cpp` `onPlayRequested` |
| 2026-08-15 | [GNOME Network Displays `src/wfd/wfd-params.c`](https://gitlab.gnome.org/GNOME/gnome-network-displays/-/blob/master/src/wfd/wfd-params.c) | CEA / VESA / HH `resolution_table` bit maps | `src/session/wfdvideomode.cpp` `kCea` / `kVesa` / `kHh` |
| 2026-08-15 | [GNOME Network Displays `src/wfd/wfd-media-factory.c`](https://gitlab.gnome.org/GNOME/gnome-network-displays/-/blob/master/src/wfd/wfd-media-factory.c) | `videoscale` to negotiated raw size before `x264enc` | `src/session/gstencoder.cpp` `startGst` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/SKILL.md` | DTK6-only app; Qt logs; no widget-layer protocol work | this change stays in `src/session` / `src/engine` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/app-dev-with-dtk.md` | CMake `add_executable` + `Qt6::Core` test helper | `CMakeLists.txt` `wfdvideomode-check` |
| 2026-08-15 | `docs/constraints.md` §5 | AAC + Pulse/PipeWire + clock sync; video-only still valid | `src/session/wfdaudiomode.cpp` `selectWfdAudioMode`; `src/session/gstencoder.cpp` `start` |
| 2026-08-15 | `docs/architecture.md` UI / Encode | Audio toggle via CastEngine; H.264 + AAC | `src/engine/castengine.cpp` `setAudioEnabled`; `src/ui/mainwindow.cpp` `setupUi` |
| 2026-08-15 | `docs/platform/x11.md` Audio | Default-sink monitor; AAC in the same MPEG-TS | `src/session/gstencoder.cpp` `desktopPulseMonitor` / `startGst` / `startFfmpeg` |
| 2026-08-15 | [GNOME Network Displays `src/wfd/wfd-params.c`](https://github.com/GNOME/gnome-network-displays/blob/master/src/wfd/wfd-params.c) | AAC bitmap: bit 0 = 48 kHz 2ch, bit 1 = 44.1 kHz 2ch | `src/session/wfdaudiomode.cpp` `kAac48k` / `kAac441k` |
| 2026-08-15 | [GNOME Network Displays `src/wfd/wfd-media-factory.c`](https://github.com/GNOME/gnome-network-displays/blob/master/src/wfd/wfd-media-factory.c) | `pulsesrc provide-clock=true` + AAC into `mpegtsmux` | `src/session/gstencoder.cpp` `startGst` audio branch |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/button.md` | `DSwitchButton` + `checkedChanged` | `src/ui/mainwindow.cpp` `setupUi` / `bindEngine` |
| 2026-08-15 | local `pactl get-default-sink` + Pulse monitor naming | System audio is `${default_sink}.monitor`, not the mic | `src/session/gstencoder.cpp` `desktopPulseMonitor` |
| 2026-08-15 | `ffmpeg -f pulse` / `aac` / `aresample=async=1` | Pulse capture, AAC-LC, light A/V drift correction | `src/session/gstencoder.cpp` `startFfmpeg` |
| 2026-08-15 | `docs/platform/x11.md` Limits | One output, not the virtual desktop union; primary default | `src/engine/castengine.cpp` `refreshDisplays` / `selectedDisplay`; `src/session/gstencoder.cpp` `ximagesrcElement` / `startFfmpeg` |
| 2026-08-15 | `docs/architecture.md` Capture backend | `start(DisplaySource)`; widgets must not grab | `src/capture/capturebackend.h`; `src/capture/x11capture.cpp` `start`; `src/ui/mainwindow.cpp` `refreshDisplayList` |
| 2026-08-15 | [Qt `QScreen`](https://doc.qt.io/qt-6/qscreen.html) | `geometry()`, `name()`, `manufacturer()` / `model()`, primary screen | `src/engine/castengine.cpp` `refreshDisplays` |
| 2026-08-15 | [GStreamer `ximagesrc`](https://gstreamer.freedesktop.org/documentation/ximagesrc/index.html) | `startx` / `starty` / `endx` / `endy` inclusive crop | `src/capture/displaysource.h` `ximagesrcRegionProperties`; `src/session/gstencoder.cpp` `ximagesrcElement` |
| 2026-08-15 | [ffmpeg x11grab](https://ffmpeg.org/ffmpeg-devices.html#x11grab) | `-video_size WxH -i :0+x,y` | `src/capture/displaysource.h` `x11grabSize` / `x11grabInputSpecifier`; `src/session/gstencoder.cpp` `startFfmpeg` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/input.md` | `DComboBox` + `currentIndexChanged` | `src/ui/mainwindow.cpp` `setupUi` / `refreshDisplayList` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/index.md` | Dropdown selection → `DComboBox` | `src/ui/mainwindow.cpp` monitor row |
| 2026-08-15 | `docs/architecture.md` UI pairing prompts | Pairing stays in CastEngine; widgets do not call NM | `src/session/nmsecretagent.*`; `src/engine/castengine.cpp` `bindPairing`; `src/ui/mainwindow.cpp` `onPairingRequested` |
| 2026-08-15 | local `man nm-settings-dbus` wifi-p2p | `wps-method` uint32; default/AUTO lets NM pick PBC or PIN | `src/session/p2psession.cpp` `activate` `wifi-p2p.wps-method` |
| 2026-08-15 | [NM AgentManager](https://networkmanager.dev/docs/api/1.44.4/gdbus-org.freedesktop.NetworkManager.AgentManager.html) | `Register` / `Unregister` identifier | `src/session/nmsecretagent.cpp` ctor / `unregisterAgent` |
| 2026-08-15 | [NM SecretAgent](https://networkmanager.dev/docs/api/1.44.4/gdbus-org.freedesktop.NetworkManager.SecretAgent.html) | `GetSecrets` `a{sa{sv}}`, delayed PIN reply, `CancelGetSecrets`, `ALLOW_INTERACTION` / `WPS_PBC_ACTIVE` | `src/session/nmsecretagent.cpp` `handleGetSecrets` / `providePin` |
| 2026-08-15 | local `man nm-settings-dbus` 802-11-wireless-security | WPS PIN is the `pin` secret | `src/session/nmsecretagent.cpp` `providePin` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/dialog.md` | `DDialog` + `addContent` + `ButtonRecommend` | `src/ui/mainwindow.cpp` `onPairingRequested` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/input.md` | `DLineEdit` alert / placeholder | `src/ui/mainwindow.cpp` `onPairingRequested` PIN field |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/application.md` | `loadTranslator()` before UI; QM basename = `applicationName` | `src/main.cpp` `main`; `CMakeLists.txt` `qt6_add_translation`; `translations/ot-cast_zh_CN.ts` / `zh_TW.ts` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/app-dev-with-dtk.md` §2.2 | Translator basename matches `applicationName`; no `tr()` for units | `src/main.cpp` `ot-cast`; `translations/ot-cast_*.ts` |
| 2026-08-15 | `/usr/share/dde-clipboard/translations/` | Install `app_zh_CN.qm` / `app_zh_TW.qm` under `share/<app>/translations` | `CMakeLists.txt` `install(... share/ot-cast/translations)` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/app-dev-with-dtk.md` §4 | `Build-Depends` DTK6 `-dev` packages; runtime via `${shlibs:Depends}` | `debian/control` |
| 2026-08-15 | `/usr/share/applications/dde-file-manager.desktop` | `Type=Application`, `Exec`, `Icon`, `Categories`, `Name[zh_CN]` | `data/com.01tool.cast.desktop` |
| 2026-08-15 | [Debian debhelper compat 13](https://manpages.debian.org/bookworm/debhelper/debhelper.7.en.html) | `dh --buildsystem=cmake` | `debian/rules` |
| 2026-08-15 | [softprops/action-gh-release](https://github.com/softprops/action-gh-release) | Tag `v*` → GitHub Release + source tarball | `.github/workflows/release.yml` |
| 2026-08-15 | `docs/architecture.md` one-binary DTK6 app | Package the same binary; no MiracleCast | `debian/control` Description |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/theme/icontheme.md` | Look up the app icon by base name `ot-cast`; fallback `video-display` | `src/main.cpp` `setProductIcon`; `src/ui/mainwindow.cpp` title bar |
| 2026-08-15 | `/usr/share/icons/hicolor/scalable/apps/dde-cooperation.svg` | Deepin-style rounded plate + content; XDG hicolor scalable | `data/icons/hicolor/scalable/apps/ot-cast.svg` |
| 2026-08-15 | `docs/architecture.md` one binary, DTK UI only | Package the same DTK sender; do not ship MiracleCast | `scripts/package.sh` |
| 2026-08-15 | [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) | AppDir + AppImage output; `--appimage-extract-and-run` without FUSE | `scripts/package.sh` `build_appimage` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/application.md` | `applicationName` = binary; unique app ID for single-instance | `src/main.cpp` `ot-cast` / `com.01tool.cast` |
| 2026-08-15 | [linuxdeploy-plugin-qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt) | Bundle Qt 6 plugins; `EXTRA_QT_PLUGINS` for styles | `scripts/package.sh` `build_appimage` |
| 2026-08-15 | local `qmake6 -query QT_INSTALL_PLUGINS` | Chameleon style, DTK icon engines, `qdeepin` platform theme | `scripts/package.sh` `copy_qt_plugin` |
| 2026-08-15 | `docs/architecture.md` two labeled transports | Capture shared; WFD first; DLNA planned; widgets off UPnP | [architecture.md](architecture.md) Layers / first cut §6; [AGENTS.md](../AGENTS.md) Product facts |
| 2026-08-15 | `docs/constraints.md` §2 | True Miracast is P2P; many sinks are immature; DLNA is same-LAN | [constraints.md](constraints.md) §2–4, §7; [protocols/README.md](protocols/README.md) When to use which |
| 2026-08-15 | `docs/feasibility.md` | DTK is UI only; DLNA is engine work, not in deepin-network-displays | [feasibility.md](feasibility.md) What DTK does not cover / Existing Deepin work |
| 2026-08-15 | `docs/platform/x11.md` Approach | Same H.264 encode; WFD sends RTP, DLNA will serve HTTP | [platform/x11.md](platform/x11.md) Approach step 4 / Requirements |
| 2026-08-15 | `docs/platform/wayland.md` Required path | Portal frames feed the same encode; transport is WFD or DLNA | [platform/wayland.md](platform/wayland.md) Required path / What the app should do now |
| 2026-08-15 | `src/engine/sinkdevice.h` `SinkDevice` | WFD-only fields today; protocol tag required before DLNA lists | [architecture.md](architecture.md) Sink identity; [protocols/dlna.md](protocols/dlna.md) Engine fit |
| 2026-08-15 | `src/engine/castengine.h` `SessionState` | Reuse Idle/Scanning/Connecting/Streaming; branch `connectToSink` | [protocols/dlna.md](protocols/dlna.md) Engine fit |
| 2026-08-15 | [UPnP AV Architecture:1](https://upnp.org/specs/av/UPnP-av-AVArchitecture-v1.pdf) | Control Point + DMR; `SetAVTransportURI` then `Play` | [protocols/dlna.md](protocols/dlna.md) Role / Required path |
| 2026-08-15 | [AVTransport:2](https://www.upnp.org/specs/av/UPnP-av-AVTransport-v2-Service.pdf) | `InstanceID` 0, `SetAVTransportURI`, `Play` Speed=1, `Stop` | [protocols/dlna.md](protocols/dlna.md) Required path |
| 2026-08-15 | [ConnectionManager:3](https://upnp.org/specs/av/UPnP-av-ConnectionManager-v3-Service-20101231.pdf) | `GetProtocolInfo` before guessing MIME | [protocols/dlna.md](protocols/dlna.md) Required path / Limits |
| 2026-08-15 | [MediaRenderer:3](https://upnp.org/specs/av/UPnP-av-MediaRenderer-v3-Device-20101231.pdf) | Device type; search still `MediaRenderer:1` | [protocols/dlna.md](protocols/dlna.md) Discovery URN |
| 2026-08-15 | [DLNA.org](https://www.dlna.org/) | `contentFeatures.dlna.org` / `transferMode.dlna.org: Streaming`; alliance dissolved | [protocols/dlna.md](protocols/dlna.md) Required path step 3; this file DLNA table |
| 2026-08-15 | [GNOME GUPnP](https://gitlab.gnome.org/GNOME/gupnp) | Preferred SOAP control-point stack | [protocols/dlna.md](protocols/dlna.md) Engine fit |
| 2026-08-15 | [GNOME GSSDP](https://gitlab.gnome.org/GNOME/gssdp) | SSDP M-SEARCH on `239.255.255.250:1900` | [protocols/dlna.md](protocols/dlna.md) Required path |
| 2026-08-15 | [GNOME gupnp-av](https://gitlab.gnome.org/GNOME/gupnp-av) | DIDL-Lite `CurrentURIMetaData` / ProtocolInfo helpers | [protocols/dlna.md](protocols/dlna.md) Engine fit |
| 2026-08-15 | [Rygel](https://gitlab.gnome.org/GNOME/rygel) | Linux DMS/DMR; do not start as this app’s UI | [protocols/dlna.md](protocols/dlna.md) Engine fit |
| 2026-08-15 | [GNOME Network Displays](https://gitlab.gnome.org/GNOME/gnome-network-displays) | Miracast + Chromecast, not DLNA; do not copy Chromecast | [feasibility.md](feasibility.md) Existing Deepin work; [protocols/README.md](protocols/README.md) out of scope |
| 2026-08-15 | [link89/dlna-cast](https://github.com/link89/dlna-cast) | Live x11grab → HLS → UPnP Play; CLI reference only | [protocols/dlna.md](protocols/dlna.md) Engine fit |
| 2026-08-15 | `AGENTS.md` Product facts | Explicit DLNA backend; never label a DMR as Miracast | [README.md](../README.md) Verdict; [protocols/README.md](protocols/README.md) |
| 2026-08-15 | `docs/protocols/dlna.md` Required path / Engine fit | SSDP MediaRenderer, HTTP TS, AVTransport Play/Stop, Qt helper | `src/discovery/dlnadiscovery.cpp`; `src/session/dlnasession.cpp`; `src/engine/castengine.cpp` `connectDlna` |
| 2026-08-15 | [UPnP AV Architecture:1](https://upnp.org/specs/av/UPnP-av-AVArchitecture-v1.pdf) | Control Point + ephemeral HTTP; not a ContentDirectory | `src/session/dlnasession.cpp` `setUriAndPlay` |
| 2026-08-15 | [AVTransport:2](https://www.upnp.org/specs/av/UPnP-av-AVTransport-v2-Service.pdf) | `SetAVTransportURI`, `Play` Speed=1, `Stop` InstanceID 0 | `src/session/dlnasession.cpp` `setUriAndPlay` / `soapStop` |
| 2026-08-15 | [ConnectionManager:3](https://upnp.org/specs/av/UPnP-av-ConnectionManager-v3-Service-20101231.pdf) | `GetProtocolInfo` Sink list | `src/session/dlnaprofile.cpp` `parseConnectionManagerSink` / `pickDlnaProfile` |
| 2026-08-15 | [DLNA.org](https://www.dlna.org/) | `transferMode.dlna.org: Streaming`, `contentFeatures.dlna.org`, `OP=00` live | `src/session/dlnasession.cpp` `httpHeaders`; `src/session/dlnaprofile.cpp` `pickDlnaProfile` |
| 2026-08-15 | `src/engine/sinkdevice.h` | `CastProtocol::Miracast` / `Dlna` plus AVTransport URLs | `src/discovery/dlnadiscovery.cpp` `toSink`; `src/ui/mainwindow.cpp` `refreshSinkList` |
| 2026-08-15 | `docs/architecture.md` first cut §6 | Merge P2P + DLNA lists; widgets stay off UPnP | `src/engine/castengine.cpp` `mergeSinks` / `startScan` |
| 2026-08-15 | `docs/platform/x11.md` Approach | Same H.264 encode; MPEG-TS on stdout for HTTP | `src/session/gstencoder.cpp` `startMpegTsPipe` |
| 2026-08-15 | `~/.agents/skills/deepin/dtk-development/references/widgets/item-delegate.md` | `DViewItemAction` subtitle + right-edge protocol label | `src/ui/mainwindow.cpp` `refreshSinkList` |
| 2026-08-15 | [Qt `QUdpSocket`](https://doc.qt.io/qt-6/qudpsocket.html) | SSDP M-SEARCH to `239.255.255.250:1900` | `src/discovery/dlnadiscovery.cpp` `sendMsearch` |
| 2026-08-15 | [Qt `QTcpServer`](https://doc.qt.io/qt-6/qtcpserver.html) | HTTP GET/HEAD for live TS | `src/session/dlnasession.cpp` `handleClient` |
