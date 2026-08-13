# References

Projects and docs that this design builds on. Prefer reuse over a new WFD implementation.

## Deepin

| Project | Role |
|---------|------|
| [linuxdeepin/deepin-network-displays](https://github.com/linuxdeepin/deepin-network-displays) | Deepin fork of GNOME Network Displays. Experimental Wi-Fi Display sender. Portal first, X11 `ximagesrc` fallback. |
| [linuxdeepin/dde-tray-loader](https://github.com/linuxdeepin/dde-tray-loader) | Ships the **无线投屏** (`wireless-casting`) quick-panel plugin and icons. |
| [linuxdeepin/xdg-desktop-portal-dde](https://github.com/linuxdeepin/xdg-desktop-portal-dde) | DDE portal backend. Confirm ScreenCast vs Screenshot on the target version. |
| [linuxdeepin/treeland](https://github.com/linuxdeepin/treeland) | DDE Wayland compositor (wlroots + QtQuick). Capture must go through it or a portal backend. |
| [DTK development skill](file:///home/playhi/.agents/skills/deepin-skills/dtk-development/SKILL.md) | App CMake, `DApplication` / `DMainWindow`, platform detection, packaging. |

deepin 23 release notes introduced wireless screen casting in the quick panel and later fixed the plugin’s incomplete options display.

## Upstream Linux senders

| Project | Role |
|---------|------|
| [GNOME Network Displays](https://gitlab.gnome.org/GNOME/gnome-network-displays) | Closest production open-source sender. Miracast + Chromecast. PipeWire portal, X11 fallback, NetworkManager P2P. Still described as experimental. |
| [MiracleCast](https://github.com/albfan/miraclecast) | Low-level WFD toolkit. Poor desktop fit (often needs NM/wpa stopped). Do not use as the app base. |
| [FluxCast](https://github.com/IlyaP358/fluxcast) | Newer Python WFD client; native wlroots path, ~1 s latency reported. Useful as a protocol reference, not as the DTK UI. |

## System pieces

| Component | Why it matters |
|-----------|----------------|
| NetworkManager ≥ 1.15.2 | P2P together with `wpa_supplicant` (not `iwd`) |
| `wpa_supplicant` | `CONFIG_P2P` + `CONFIG_WIFI_DISPLAY` |
| GStreamer + `gst-rtsp-server` | Capture, H.264, RTSP/RTP |
| PipeWire + WirePlumber | Wayland (and modern X11) audio/video capture |
| `xdg-desktop-portal` | ScreenCast session on Wayland |

## Capture notes

- GNOME Network Displays README: stream the selected monitor if the mutter screencast portal is available; otherwise fall back to X11 frame grabbing.
- Arch Wiki XDG Desktop Portal table (verify when targeting a DDE release): `xdg-desktop-portal-dde` Screenshot yes, ScreenCast historically no.
- X11 `ximagesrc` lives in `gstreamer1.0-plugins-good`. Missing that plugin is a common “fallback to X11 failed” cause.

## Related local docs

- [feasibility.md](feasibility.md)
- [architecture.md](architecture.md)
- [platform/x11.md](platform/x11.md)
- [platform/wayland.md](platform/wayland.md)
- [constraints.md](constraints.md)

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
