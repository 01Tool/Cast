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
3. Connect the selected monitor. Try video-only first, then audio.
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

## DLNA

| Date | Brand | Model | Firmware | Hint | Verdict | Audio | Notes / `device-matrix` line |
|------|-------|-------|----------|------|---------|-------|------------------------------|
| 2026-08-18 | Tmall / YunOS | MagicBox_M18 (`我的天猫魔盒`) | Youku Taitan 13.8.1.2 | `live-ts-likely` (`video/mpeg` plus mp4/avi) | `live-ts` | not tried | `192.168.31.8:7300`. 720p looked soft from Mi27 4K. 1920×1080@30 H.264 main 8 Mbit also PLAYING/OK. First GET is a short probe, then reconnect. Need `yuv420p`. |

## Adding a row

Copy a table row. Date is `YYYY-MM-DD`. Do not invent models. If you only ran scan, use `untested` and paste the hint. If Play failed because the DMR wants a finite MP4, verdict is `file-only` even when the hint was `unknown`.
