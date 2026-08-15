# Miracast / Wi-Fi Display

True Miracast is a **Wi-Fi Direct** session, then WFD RTSP (TCP 7236) and RTP. It does not need a shared access point. It often drops or splits the laptop’s STA Wi-Fi.

## Why it is not enough on its own

Linux WFD quality is dominated by the Wi-Fi chipset, `wpa_supplicant` (`CONFIG_P2P` + `CONFIG_WIFI_DISPLAY`), and how the TV implements RTSP. Many adapters advertise P2P and still fail. Many TVs that show a “Miracast” logo still reject open-source senders. See [../constraints.md](../constraints.md).

Cast therefore treats Miracast as **one** backend. When the phy or the sink cannot form a WFD group, the user should be able to pick a **DLNA** renderer on the same LAN instead of being stuck. The engine must not silently retry a failed WFD session as DLNA.

## What is implemented

NetworkManager P2P scan, WPS PIN / PBC pairing, WFD RTSP, X11 grab of the selected monitor, H.264 + optional AAC in MPEG-TS over RTP. Wayland capture is stubbed.

Details: [../architecture.md](../architecture.md), [../platform/x11.md](../platform/x11.md).
