# Miracast / Wi-Fi Display

True Miracast is Wi-Fi Display: WFD RTSP (TCP 7236) and RTP. The original path forms a **Wi-Fi Direct** group and does not need a shared access point. That often drops or splits the laptop’s STA Wi-Fi.

Windows Connect and many Android phones on the **same LAN** use **MS-MICE** (Miracast over Infrastructure) instead of WPS/P2P: TCP **7250** `SOURCE_READY`, then the sink dials the same WFD RTSP :7236. That is still Miracast, not DLNA. Do not label a DMR as Miracast.

## Why it is not enough on its own

Linux WFD quality is dominated by the Wi-Fi chipset, `wpa_supplicant` (`CONFIG_P2P` + `CONFIG_WIFI_DISPLAY`), and how the TV implements RTSP. Many adapters advertise P2P and still fail. Many TVs that show a “Miracast” logo still reject open-source senders. See [../constraints.md](../constraints.md).

Cast therefore treats Miracast as **one** backend. When the phy or the sink cannot form a WFD group, the user should be able to pick a **DLNA** renderer on the same LAN instead of being stuck. The engine must not silently retry a failed WFD session as DLNA.

## What is implemented

NetworkManager P2P scan, WPS PIN / PBC pairing, WFD RTSP, X11 grab of the selected monitor, H.264 + optional AAC in MPEG-TS over RTP. Wayland capture is stubbed.

Same-LAN **MS-MICE**: mDNS `_display._tcp`, neighbor-table match of the P2P MAC (including the locally-administered bit Windows sets on the P2P address), TCP 7250 `SOURCE_READY` / `STOP_PROJECTION`, then the existing WFD server on :7236. Connect tries this first; if 7250 is closed it falls back to Wi-Fi Direct. No PIN/DTLS on this cut — that matches Windows “Projecting to this PC” with PIN off, which is how an Android phone can join on first connect.

X11 grab uses **physical** pixels (`QScreen::geometry() × devicePixelRatio()`). A 4K panel at 200% scale is 1920×1080 in Qt and 3840×2160 for `x11grab`; using the logical size only captures the top-left quarter.

`wfd_video_formats` can list several H.264 codec blocks (CEA 16:9 **and** VESA 16:10 on Windows). Cast prefers a progressive ≤30 fps mode whose aspect ratio matches the captured monitor, honors hex `max-hres`/`max-vres`, and **letterboxes** instead of stretching.

WFD says the **sink** opens TCP 7236 to the source. Many Android / MediaTek sinks become P2P GO and instead wait for the source to dial **GO:7236**. After the group is up Cast still listens, and also tries the peer/gateway IPv4.

Details: [../architecture.md](../architecture.md), [../platform/x11.md](../platform/x11.md).
