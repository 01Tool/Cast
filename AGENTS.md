# Agent instructions

Mandatory rules for every agent working in this repository. Read this file and the docs listed below before changing code or design.

## Required reading

| Path | Why |
|------|-----|
| [README.md](README.md) | Verdict and document map |
| [docs/feasibility.md](docs/feasibility.md) | What DTK covers vs the WFD stack |
| [docs/architecture.md](docs/architecture.md) | Layers, `CastEngine`, first implementation cut |
| [docs/platform/README.md](docs/platform/README.md) | Capture backend split |
| [docs/platform/x11.md](docs/platform/x11.md) | X11 grab (`ximagesrc` / XShm) |
| [docs/platform/wayland.md](docs/platform/wayland.md) | Portal + PipeWire; DDE ScreenCast gap |
| [docs/constraints.md](docs/constraints.md) | Chipset, P2P vs DLNA, sinks, latency, audio |
| [docs/protocols/README.md](docs/protocols/README.md) | Miracast vs DLNA transports |
| [docs/references.md](docs/references.md) | Known projects **and** the usage log you must update |

DTK conventions: `~/.agents/skills/deepin-skills/dtk-development/SKILL.md` and its `references/`. Load that skill when writing or packaging the app.

## Product facts (do not violate)

- This is a **DTK sender** (product name **Cast**, binary `ot-cast`) that mirrors the local screen.
- **Miracast / WFD** is the first transport (Wi-Fi Direct + RTSP + RTP). **DLNA DMR** is the same-LAN fallback. Both may appear in one device list; each row must show its protocol.
- **DTK is the UI only.** Discovery, P2P, WFD/RTSP, UPnP, capture, encode, and send live in `CastEngine` and backends — not in widgets.
- **Reuse**, do not rewrite WFD. Start from `linuxdeepin/deepin-network-displays` / GNOME Network Displays. Do **not** base the desktop app on MiracleCast.
- **One binary, two capture backends.** Detect the session with `DGuiApplicationHelper::IsXWindowPlatform` / `IsWaylandPlatform`.
- **X11 first.** Implement `X11Capture`. Stub `PortalCapture` until DDE ScreenCast (or equivalent) exists.
- **Never X11-grab on Wayland.** That only sees XWayland windows. Fail with a clear error if ScreenCast is missing.
- Widgets must not call X11, portal, NetworkManager, `wpa_supplicant`, UPnP/SSDP, or GStreamer APIs directly.
- True Miracast is **Wi-Fi Direct**, not “same LAN then stream.” DLNA is allowed as an **explicit** backend. Do not label a DMR as Miracast. Do not add Chromecast under either name.
- Do not claim universal sink support or “low latency” without a measured device matrix.
- Video-only is a valid first release. Audio (AAC + sync) is later.

## DTK / engineering

- DTK6 + Qt 6. Use `DApplication` and `DMainWindow`, not `QApplication` / `QMainWindow`.
- Forward headers without `.h`: `#include <DApplication>`, `#include <DGuiApplicationHelper>`.
- Log with Qt macros (`qInfo`, `qWarning`, `qDebug`) after `DLogManager` appenders. Do not use dtklog `dDebug`.
- CMake: `Dtk6::Core`, `Dtk6::Gui`, `Dtk6::Widget` (plus GStreamer / libnm / portal when the engine is wired).
- Prefer existing Deepin pieces (`deepin-network-displays`, `dde-tray-loader` `wireless-casting`) over a new protocol stack.

## Commits

Every commit created or proposed by an agent **must** include a `Co-Authored-By` trailer for the **agent**, not the human git user:

```
Co-Authored-By: Grok 4.6 <grok@grok.ai>
```

Rules:

- **Full Name** is the agent/model name (example: `Grok 4.6`). **Email** is the agent address (example: `grok@grok.ai`). Do not put the human author here.
- If a different agent makes the commit, use that agent’s own name and email in the same `Co-Authored-By: Full Name <Email>` shape.
- Put the trailer on its own line at the end of the message, with a blank line before it. Do not hide it in the subject.
- Do not commit if the message lacks this trailer.
- Do not `--amend` or force-push unless the user asked.
- Keep the subject imperative and scoped (e.g. `Add X11 capture backend`).

Example:

```
Add CastEngine session state machine

Introduce Idle/Scanning/Connecting/Streaming/Failed and
wire DTK UI signals only through CastEngine.

Co-Authored-By: Grok 4.6 <grok@grok.ai>
```

## References (mandatory)

If an agent reads, copies, or relies on **any** document, repository, header, spec, wiki, blog, issue, or local skill, it must record that source before the task is done.

Update [docs/references.md](docs/references.md):

1. **Catalog** — If the source is new, add it under the matching section (Deepin, upstream, system, local docs, or a new section). Include a stable URL or path and a one-line role.
2. **Where used** — Append one row per use to the **Where used** table. Every row must name:
   - **Source** — full URL or repo path (not just “GNOME docs”)
   - **What was used** — file, symbol, section, or idea taken
   - **Used in** — this repo’s path and section/function (e.g. `src/capture/x11capture.cpp` `X11Capture::start`, or `docs/platform/x11.md` §Approach)
   - **Date** — `YYYY-MM-DD`

Do not finish a change that used outside or in-repo references without those rows. A code comment is not a substitute. “Inspired by X” without a path is not enough.

If you only followed this repo’s own docs, still log them (source = the local path).

## Task completion

A task is not done until:

1. The change matches [docs/architecture.md](docs/architecture.md) and the platform rules.
2. [docs/references.md](docs/references.md) catalog + **Where used** are updated for every source you touched.
3. Any commit message includes `Co-Authored-By: Grok 4.6 <grok@grok.ai>` (or that agent’s own name and `grok@grok.ai`-style email).
