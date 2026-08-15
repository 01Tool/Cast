# DLNA / UPnP AV

DLNA Digital Media Renderer (DMR) is a **same-LAN** path. The laptop stays on the access point. The TV pulls an HTTP media URL after a UPnP `SetAVTransportURI`. That is not Miracast. The UI must label these devices **DLNA**.

Many living-room TVs implement DMR more completely than Wi-Fi Display. Linux WFD then fails on the chipset, `wpa_supplicant` (`CONFIG_WIFI_DISPLAY`), or the TV’s RTSP — while “Play To” from a phone already works. DLNA is the fallback for that majority case. The trade-off is extra buffering and a smaller set of live-stream profiles.

## Role

UPnP AV splits three jobs. Cast takes two of them for a **single live URL**; the TV takes the third.

| Role | Who | What Cast does |
|------|-----|----------------|
| Digital Media Renderer (DMR) | The TV / box | Plays the URL we give it |
| Control Point (DMC) | Cast | SSDP search, `SetAVTransportURI`, `Play`, `Stop` |
| Media source | Cast (ephemeral HTTP) | Serves the encoded desktop. **Not** a ContentDirectory library |

Do not browse the TV’s media. Do not ask the user to export a file and “open it on the TV.”

## Required path

```
SSDP M-SEARCH  →  MediaRenderer:1 + AVTransport:1
capture + H.264 (+ optional AAC)   (same as WFD)
local HTTP     →  MPEG-TS or fMP4 + DLNA streaming headers
SOAP           →  SetAVTransportURI + Play
TV GET         →  pulls the URL on the STA LAN
```

Discovery is SSDP (`239.255.255.250:1900`) for `urn:schemas-upnp-org:device:MediaRenderer:1`. Control is the device’s `urn:schemas-upnp-org:service:AVTransport:1`. Optional `ConnectionManager:1` supplies `GetProtocolInfo` so the MIME/profile is not a guess. Media is **not** RTP into a P2P group.

Live desktop is not a file. The sender must:

1. Encode the selected monitor (and optional AAC) with the same capture/encode path as WFD.
2. Bind HTTP on the laptop’s **STA IPv4** (the address the TV can route to). Advertise that host in the URI.
3. Answer `GET`/`HEAD` with `transferMode.dlna.org: Streaming` and `contentFeatures.dlna.org` (and honor `getcontentFeatures.dlna.org` on the request).
4. Call `SetAVTransportURI` (`InstanceID` 0, `CurrentURI`, DIDL-Lite `CurrentURIMetaData`) then `Play` (`Speed` 1).
5. `Stop` and close HTTP on disconnect.

## Why this is the fallback

| Miracast / WFD | DLNA DMR |
|----------------|----------|
| Needs P2P + `CONFIG_WIFI_DISPLAY` | Needs a shared IPv4 LAN |
| Often breaks STA Wi-Fi | Leaves STA Wi-Fi alone |
| ~200 ms–1 s when it works | Often **1–5 s** (renderer buffer) |
| Sink matrix is small on Linux | More TVs expose a DMR |
| Fine for slides if the group stays up | Fine for slides; worse for games |

If both a WFD IE and a DMR appear for the same TV, list **two rows** (or one row with two actions). Do not merge them into “Miracast.” Do not auto-retry a failed WFD session as DLNA without the user choosing the DLNA row.

## Limits

- Some DMRs only play finite files (MP4 with a known duration). Live MPEG-TS / fMP4 will fail; report that as a sink limit, not as “DLNA is broken.”
- `ProtocolInfo` must match what the renderer advertised (`http-get:*:video/mpeg:*` or similar). Guessing `video/mp4` is a common reject.
- Multi-homed laptops: the URI host must be the address on the **same L3 network as the TV**, not `127.0.0.1` and not a leftover P2P address.
- Firewall rules that block inbound HTTP on the laptop will stop the TV from pulling the stream.
- Latency is not “low.” Do not advertise it as such.
- Audio follows the container (AAC in TS is a reasonable first try). Sync is the renderer’s clock, not WFD RTSP. There is no `wfd_audio_codecs` bitmap.
- IPv4 first. SSDP on IPv6-only LANs is a later problem.

## Engine fit

Keep widgets off UPnP. `CastEngine` already owns `Idle` / `Scanning` / `Connecting` / `Streaming` / `Failed`. DLNA reuses those states.

| Piece | Role |
|-------|------|
| `SinkDevice` | `protocol` is `Miracast` or `Dlna`; DLNA rows carry `avTransportUrl` |
| `DlnaDiscovery` | SSDP search; merge MediaRenderers into `CastEngine::sinks()` |
| `DlnaSession` | HTTP media server + AVTransport Play/Stop |
| Shared capture / encode | Same `DisplaySource` + H.264 path as WFD; mux to HTTP instead of RTP |

`connectToSink` branches on the tag. Pairing prompts stay P2P-only.

First cut uses a **Qt SSDP + SOAP helper** (`QUdpSocket` + `QNetworkAccessManager`) so the DTK app does not grow a GLib main loop. GUPnP / GSSDP / gupnp-av remain acceptable if a later cut needs them. Do not start Rygel or another desktop UI. Do not route this through MiracleCast. GNOME Network Displays speaks Miracast and Chromecast, not DLNA — do not copy its Chromecast backend under this name. `dlna-cast` (ffmpeg + HLS + UPnP) is a CLI proof that live desktop→DMR works; this cut serves MPEG-TS so the existing encoder stays shared. HLS is a sink-specific fallback, not the default.

## First DLNA cut

Implemented:

1. Scan and list MediaRenderers next to P2P sinks, tagged **DLNA**.
2. Connect = HTTP + `SetAVTransportURI` + `Play` for the selected monitor (AAC in TS when the toggle is on).
3. Disconnect = `Stop` and tear down HTTP.

Still open: a measured device matrix (which TVs accept live TS vs which need a file-like stream). Video is capped at 1280×720@30.

Chromecast, AirPlay, and “custom TCP on the same Wi-Fi” stay out of this document.
