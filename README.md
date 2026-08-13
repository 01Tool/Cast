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
