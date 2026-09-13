# Device matrix

Cast does not claim “works with every Miracast / DLNA TV.” Fill the tables below from **measured** sessions on this sender. A logo, a store page, or `GetProtocolInfo` alone is not a row.

Keep **two** lists. A TV that fails Wi-Fi Display can still be a usable DMR, and the reverse.

This sender’s current payloads:

| Path | What the TV must accept |
|------|-------------------------|
| Miracast | WFD RTSP :7236 + H.264 (optional AAC) in MPEG-TS over RTP. Same-LAN Windows Connect also needs TCP 7250 (`SOURCE_READY`) before RTSP. |
| DLNA | HTTP pull of **live** MPEG-TS (`video/mpeg`), H.264 main ≤1920×1080@30, optional AAC. No `Content-Length`. `DLNA.ORG_OP=00` (no seek). `MPEG_TS_HD_NA_ISO` when the output is HD. |

## How to test

1. X11 session. Note adapter (`iw phy`), `wpa_supplicant` WFD (`gdbus … WFDIEs`), and whether STA Wi-Fi stays up.
2. Scan. Confirm the row is labeled **Miracast** or **DLNA**. Do not merge them. A Windows Connect PC may show **Miracast · LAN** (MS-MICE) as well as a P2P MAC.
3. Connect the selected monitor. Try video-only first, then audio. Optionally send a local file (DLNA HTTP, or Miracast transcode) instead of the screen.
4. Record the verdict and a short note (pairing, RTSP reject, no HTTP GET, black screen, …).
5. Paste the `device-matrix` line from the log (`~/.cache/ot-cast/` or the console). The engine prints one on scan classify, stream start, and failure.

### Verdicts

| Token | Meaning |
|-------|---------|
| `streaming` | Picture on the sink |
| `no-wfd-ie` | P2P peer, empty WFD IEs |
| `p2p-timeout` | No group / no RTSP |
| `mice-timeout` | TCP 7250 or LAN RTSP never reached Streaming |
| `live-ts` | DLNA pulled `/cast.ts` and played |
| `file-only` | DMR plays files; live TS rejected or never fetched |
| `no-get` | `Play` ok, TV never HTTP-GETs the laptop |
| `uri-reject` | `SetAVTransportURI` or `Play` fault |
| `untested` | Seen on the LAN, not connected |

## ProtocolInfo hint (not a measurement)

On scan, if the renderer has `ConnectionManager`, Cast calls `GetProtocolInfo` and classifies the **Sink** list:

| Hint | Rule |
|------|------|
| `live-ts-likely` | At least one `http-get` MIME/profile looks like MPEG-TS (`video/mpeg`, `video/mp2t`, `video/vnd.dlna.mpeg-tts`, `MPEG_TS_*`, `AVC_TS_*`) |
| `file-only-likely` | Video entries exist, but they are file containers (`video/mp4`, `AVC_MP4_*`, AVI/WMV, HLS) and **no** TS profile |
| `no-video` | No `http-get` video |
| `unknown` | No `Sink` list, or only unrecognized types |

The UI may show that hint on the DLNA row. Testers still write a measured verdict. A `live-ts-likely` Sony that then never GETs is `no-get`, not a pass.

HLS (`application/vnd.apple.mpegurl`) is noted in the summary. This cut does not serve HLS.

## Miracast

| Date | Brand | Model | Adapter / phy | WFD IEs | Verdict | Audio | Latency (ms) | Notes |
|------|-------|-------|---------------|---------|---------|-------|--------------|-------|
| 2026-08-27 | Microsoft (Windows Connect) | `DESKTOP-CDKV2MA` | Intel AX210 `wlp4s0` | yes `00000600111c440006` (sink, RTSP 7236) | `streaming` | AAC 48 kHz | — | Android first-connect with no PIN is **MS-MICE**, not WPS. STA `192.168.196.16`. Cast: mDNS + ARP LAA MAC → TCP 7250 → WFD RTSP. HiDPI grab is 3840×2160. Windows lists VESA 16:10 **and** CEA 16:9; pick 1920×1080@30 to match Mi 27, letterbox if needed. Dual-stack PLAY must yield IPv4 for ffmpeg RTP. |
| 2026-08-30 | Xiaomi (MediaTek) | Pad 7S Pro 12.5 | Intel AX210 `wlp4s0` | yes `00000600111c440032` (primary sink, RTSP 7236) | `streaming` | LPCM 48 kHz | — | P2P MAC `36:E5:24:A6:07:5E`. No MS-MICE. Pad is P2P GO; this PC is client `192.168.49.128`. Source dials GO `192.168.49.1:7236`. Empty `Sender:` was empty WPS `DeviceName`. 1920×1080@30 RTP 15550. 2026-09-06: `wfd_audio_codecs: LPCM 00000002 00` (no AAC). Encoder `ffmpeg -c:a pcm_bluray` to `rtp://192.168.49.1:15550`. Status: `1920x1080@30 + LPCM 48 kHz`. |
| 2026-09-08 | Xiaomi (MediaTek) | Pad 7S Pro 12.5 | Intel AX210 `wlp4s0` | yes (scan) | — | — | — | **Treeland, not a WFD verdict.** Scan listed this Pad. ScreenCast picker (DP-1) + Allow worked. Cast froze on “Select a screen to share…” because capture used the generic Wayland `PortalCapture` path. Dock system indicator: `正在共享屏幕至[]` (empty name; not `ot-cast-tray`). No P2P prompt on the Pad; no `pipewiresrc`. See [platform/treeland.md](platform/treeland.md). X11 row above still stands. |
| 2026-09-13 | Xiaomi (MediaTek) | Pad 7S Pro 12.5 | Intel AX210 `wlp4s0` | yes (P2P) | `streaming` | **further investigation** | — | **Treeland screen cast OK.** User: picture looks great (`1920x1080@30` DP-1). Pad GO `192.168.49.1`, PC `192.168.49.128`, MAC `36:E5:24:A6:07:5E`, RTP `15550`. Audio is **not** a pass: sink `LPCM 00000002 00` (no AAC). ffmpeg `pcm_bluray` was silent. SET AAC (gst `voaacenc`) while the sink listed only LPCM: user heard a loud boom (AAC decoded as PCM). Keep LPCM in SET_PARAMETER. X11 LPCM row still stands. See [platform/treeland.md](platform/treeland.md) §WFD audio. |

## DLNA

| Date | Brand | Model | Firmware | Hint | Verdict | Audio | Notes / `device-matrix` line |
|------|-------|-------|----------|------|---------|-------|------------------------------|
| 2026-09-06 | Tmall / YunOS | MagicBox_M18 (`我的天猫魔盒`) | Youku Taitan 13.8.1.2 | `live-ts-likely` (`video/mpeg` plus mp4/avi) | `live-ts` | AAC muxed, not ear-checked | **X11.** `192.168.31.8:7300`. GET `http://192.168.31.46:44757/cast.ts`. `GetTransportInfo` = `PLAYING` / `OK`. One ffmpeg for >90 s (no probe restart). ~64 MiB TS acked at ~8 Mbit. 1920×1080@30 H.264 main + AAC 48 kHz. Need `yuv420p`. |
| 2026-09-13 | Tmall / YunOS | MagicBox_M18 (`我的天猫魔盒`) | Youku Taitan 13.8.1.2 | `live-ts-likely` | `live-ts` | AAC 48 kHz, ear-checked | **Treeland.** `WAYLAND_DISPLAY=treeland.socket`. `TreelandCapture` + `pipewiresrc fd=3 always-copy=true` (no `path=`). RGBx 3840×2160 → 1920×1080 H.264 main Annex-B + Pulse monitor `voaacenc` (`pulsesrc provide-clock=false`). User: picture and system audio on the box. Same-day video-only cut first (mux stall); AAC after single-clock mux. `gst-launch` ~260% CPU with AAC. Not a latency claim. X11 ffmpeg row above still stands. |

## Adding a row

Copy a table row. Date is `YYYY-MM-DD`. Do not invent models. If you only ran scan, use `untested` and paste the hint. If Play failed because the DMR wants a finite MP4, verdict is `file-only` even when the hint was `unknown`.
