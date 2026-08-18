# Device matrix

Cast does not claim “works with every Miracast / DLNA TV.” Fill the tables below from **measured** sessions on this sender. A logo, a store page, or `GetProtocolInfo` alone is not a row.

Keep **two** lists. A TV that fails Wi-Fi Display can still be a usable DMR, and the reverse.

This sender’s current payloads:

| Path | What the TV must accept |
|------|-------------------------|
| Miracast | WFD RTSP :7236 + H.264 (optional AAC) in MPEG-TS over RTP |
| DLNA | HTTP pull of **live** MPEG-TS (`video/mpeg`), H.264 baseline ≤1280×720@30, optional AAC. No `Content-Length`. `DLNA.ORG_OP=00` (no seek). |

## How to test

1. X11 session. Note adapter (`iw phy`), `wpa_supplicant` WFD (`gdbus … WFDIEs`), and whether STA Wi-Fi stays up.
2. Scan. Confirm the row is labeled **Miracast** or **DLNA**. Do not merge them.
3. Connect the selected monitor. Try video-only first, then audio.
4. Record the verdict and a short note (pairing, RTSP reject, no HTTP GET, black screen, …).
5. Paste the `device-matrix` line from the log (`~/.cache/ot-cast/` or the console). The engine prints one on scan classify, stream start, and failure.

### Verdicts

| Token | Meaning |
|-------|---------|
| `streaming` | Picture on the sink |
| `no-wfd-ie` | P2P peer, empty WFD IEs |
| `p2p-timeout` | No group / no RTSP |
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
| — | — | — | — | — | *none measured in this repo* | — | — | Add a row after a real session |

## DLNA

| Date | Brand | Model | Firmware | Hint | Verdict | Audio | Notes / `device-matrix` line |
|------|-------|-------|----------|------|---------|-------|------------------------------|
| — | — | — | — | — | *none measured in this repo* | — | Live TS vs file-only is the first question |

## Adding a row

Copy a table row. Date is `YYYY-MM-DD`. Do not invent models. If you only ran scan, use `untested` and paste the hint. If Play failed because the DMR wants a finite MP4, verdict is `file-only` even when the hint was `unknown`.
