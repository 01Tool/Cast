# Cast transports

Capture (X11 / Wayland) is shared. How frames reach the TV is not. The UI must show the protocol on every device row. Never list a DLNA renderer as a Miracast sink. Never switch protocol silently.

| Protocol | Document | Network | Status |
|----------|----------|---------|--------|
| Miracast / Wi-Fi Display | [miracast.md](miracast.md) | Wi-Fi Direct + RTSP + RTP | Implemented (X11 video, optional AAC). |
| DLNA / UPnP AV | [dlna.md](dlna.md) | Same LAN, SSDP + HTTP + AVTransport | First cut (X11 MPEG-TS + AVTransport). |

## When to use which

Many living-room TVs print a Miracast logo and still reject Linux WFD senders (chipset, `CONFIG_WIFI_DISPLAY`, RTSP order, HDCP). Those same TVs often expose a Digital Media Renderer that already plays HTTP video from a phone or NAS.

| Situation | Use |
|-----------|-----|
| TV / dongle has a working WFD IE and the phy can form a P2P group | **Miracast** (no shared AP required) |
| P2P missing, STA Wi-Fi must stay up, or WFD handshake fails | **DLNA** on the current LAN |
| Same TV shows both a WFD peer and a DMR | Two rows (or two actions). User picks. |
| Device is Chromecast / AirPlay only | Out of scope. Do not fake it as either name. |

Chromecast is a third protocol (not DLNA). Do not add it under either name.
