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

## 2. True Miracast is P2P, not “same LAN”

WFD typically forms a Wi-Fi Direct group between source and sink. That often drops or splits the current AP connection.

“Search devices on the same Wi-Fi, then stream over the LAN” is a different (easier) product. Some UIs blur the two. This project should say which one it implements.

- **Miracast / WFD (this project):** P2P + RTSP + RTP. Works without a shared AP. Disrupts STA Wi-Fi more often.
- **Same-network cast (not Miracast):** DLNA, Chromecast, or a custom TCP stream. Needs a shared LAN. Does not satisfy “Miracast wireless display.”

## 3. Sink compatibility

Samsung, LG, Xiaomi, and cheap HDMI dongles implement WFD differently (RTSP order, CEA/VESA modes, HDCP assumptions, audio). Expect a **device matrix**, not “any Miracast TV.”

Keep a tested-sink list as the product grows. Do not claim universal compatibility in the UI.

## 4. Latency

Typical open-source senders land around **~200 ms–1 s**. Fine for slides and documents. Poor for video and gaming unless hardware encode and a tight pipeline are invested in.

Do not advertise “low latency” until measured on target hardware.

## 5. Audio

Audio is extra work: AAC (or the sink’s codec), clock sync, and a capture source (PulseAudio / PipeWire). Video-only is still valid: the app sends AAC-LC only when the user enables system audio **and** the sink lists AAC in `wfd_audio_codecs`. Otherwise the session stays video-only.

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
