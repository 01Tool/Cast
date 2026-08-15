# Constraints

These limits decide whether users call the feature “working.” They are independent of DTK.

## 1. Wi-Fi chipset and firmware

Many adapters advertise P2P and still fail Wi-Fi Display. Driver / firmware quality is the most common Linux Miracast failure.

`wpa_supplicant` must be built with:

- `CONFIG_P2P`
- `CONFIG_WIFI_DISPLAY`

NetworkManager must manage both the main Wi-Fi device and the P2P device. `iwd` is not a supported P2P path for the GNOME / deepin-network-displays style stack.

Check before promising a device:

```bash
# P2P capability on the phy
iw phy

# wpa_supplicant WFD support (fails if CONFIG_WIFI_DISPLAY is missing)
sudo gdbus call --system \
  --dest fi.w1.wpa_supplicant1 \
  --object-path /fi/w1/wpa_supplicant1 \
  --method org.freedesktop.DBus.Properties.Get \
  fi.w1.wpa_supplicant1 WFDIEs
```

## 2. Miracast is P2P; DLNA is same-LAN

WFD typically forms a Wi-Fi Direct group between source and sink. That often drops or splits the current AP connection. Linux success also depends on the chipset and the TV’s RTSP. **Many devices do not implement Miracast well enough to be the only path.**

Cast therefore ships (and will ship) **two named protocols**:

- **Miracast / WFD:** P2P + RTSP + RTP. Works without a shared AP. Disrupts STA Wi-Fi more often. Implemented first.
- **DLNA / UPnP AV:** SSDP + HTTP + AVTransport on the existing LAN. Leaves STA Wi-Fi up. Planned because more TVs expose a Digital Media Renderer than a reliable WFD sink. See [protocols/dlna.md](protocols/dlna.md).

The UI must say which protocol a row uses. Do not list a DMR under “Miracast.” Chromecast and custom TCP are still different products; do not add them under either name.

## 3. Sink compatibility

Samsung, LG, Xiaomi, and cheap HDMI dongles implement WFD differently (RTSP order, CEA/VESA modes, HDCP assumptions, audio). DLNA renderers differ on live HTTP (some only play finite files). Keep **separate** tested-sink lists for Miracast and DLNA. Do not claim universal compatibility.

## 4. Latency

Typical open-source **WFD** senders land around **~200 ms–1 s**. DLNA live pull is often **1–5 s** because the TV buffers HTTP. Both are fine for slides. Neither is “low latency” until measured.

## 5. Audio

Audio is extra work: AAC (or the sink’s codec), clock sync, and a capture source (PulseAudio / PipeWire). Video-only is still valid.

On **Miracast**, the app sends AAC-LC only when the user enables system audio **and** the sink lists AAC in `wfd_audio_codecs`. Otherwise the WFD session stays video-only.

On **DLNA** (planned), there is no WFD codec bitmap. Audio is whatever the HTTP container and the renderer’s `ProtocolInfo` allow (AAC in MPEG-TS is the first try). Sync is the TV’s clock.

GNOME Network Displays is one of the few Linux senders that attempts synchronized audio. This app follows that path (Pulse monitor + AAC in the same MPEG-TS). MiracleCast-style tools often skip audio or route it separately.

## 6. Wrong base: MiracleCast

[MiracleCast](https://github.com/albfan/miraclecast) is a low-level WFD toolkit. It often requires stopping NetworkManager and `wpa_supplicant`, and it is not a good desktop UX base.

Prefer:

- GNOME Network Displays / `deepin-network-displays` for the WFD + GStreamer path
- NetworkManager P2P so the rest of the desktop keeps a network stack

## 7. Security and permissions

- X11 grab can capture the whole desktop without a user picker.
- Wayland capture should go through the portal so the user consents and can choose a monitor.
- P2P groups are a new L2 network; firewall rules that assume “only the AP” will break the RTP/RTSP path (a common GNOME Network Displays support issue).
- DLNA needs the opposite: the TV must open HTTP back to the laptop on the STA LAN. Outbound-only firewalls will fail `Play`.
- Many sinks use WPS push-button; others show an 8-digit PIN on the TV. The app registers an in-process NetworkManager SecretAgent so those prompts stay in the DTK window instead of depending on nm-applet.
