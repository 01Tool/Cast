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

## 2. Miracast is WFD; DLNA is same-LAN HTTP

Classic WFD forms a Wi-Fi Direct group. That often drops or splits the current AP connection. Linux success also depends on the chipset and the TV’s RTSP. **Many devices do not implement P2P Miracast well enough to be the only path.**

Windows 10+ “Projecting to this PC” and many Android phones on the **same Wi-Fi** use **MS-MICE** (TCP 7250, then the same WFD RTSP :7236). No WPS PIN when the sink has PIN off. That is still Miracast. It is not DLNA.

Cast therefore ships **two named protocols**:

- **Miracast / WFD:** P2P + RTSP + RTP, **or** MS-MICE on the current LAN then the same RTSP + RTP. P2P works without a shared AP. MICE leaves STA Wi-Fi up.
- **DLNA / UPnP AV:** SSDP + HTTP + AVTransport on the existing LAN. Leaves STA Wi-Fi up. Shipped because more TVs expose a Digital Media Renderer than a reliable WFD sink. See [protocols/dlna.md](protocols/dlna.md).

The UI must say which protocol a row uses. Do not list a DMR under “Miracast.” Chromecast and custom TCP are still different products; do not add them under either name.

## 3. Sink compatibility

Samsung, LG, Xiaomi, and cheap HDMI dongles implement WFD differently (RTSP order, CEA/VESA modes, HDCP assumptions, audio). DLNA renderers differ on live HTTP (some only play finite files). Keep **separate** tested-sink lists in [devices.md](devices.md). A ProtocolInfo hint is not a measurement. Do not claim universal compatibility.

## 4. Latency

Typical open-source **WFD** senders land around **~200 ms–1 s**. DLNA live pull is often **1–5 s** because the TV buffers HTTP. Both are fine for slides. Neither is “low latency” until measured.

## 5. Audio

Audio is extra work: AAC (or the sink’s codec), clock sync, and a capture source (PulseAudio / PipeWire). Video-only is still valid.

On **Miracast**, the app sends audio when the user enables system audio **and** the sink lists a codec we mux: **AAC-LC** if present, otherwise **LPCM** (44.1 or 48 kHz stereo). Otherwise the WFD session stays video-only.

On **DLNA**, there is no WFD codec bitmap. Audio is whatever the HTTP container and the renderer’s `ProtocolInfo` allow (AAC in MPEG-TS is the first try). Sync is the TV’s clock.

GNOME Network Displays is one of the few Linux senders that attempts synchronized audio, but it still muxes AAC only. This app follows that Pulse monitor + MPEG-TS path and adds LPCM when the sink has no AAC (Xiaomi Pad 7S Pro 12.5, EZCast-style dongles).

## 6. Wrong base: MiracleCast

[MiracleCast](https://github.com/albfan/miraclecast) is a low-level WFD toolkit. It often requires stopping NetworkManager and `wpa_supplicant`, and it is not a good desktop UX base.

Prefer:

- GNOME Network Displays / `deepin-network-displays` for the WFD + GStreamer path (including MS-MICE since 0.91)
- NetworkManager P2P so the rest of the desktop keeps a network stack

## 7. Security and permissions

- X11 grab can capture the whole desktop without a user picker.
- Treeland capture goes through the ScreenCast portal so the user consents and can choose a monitor. Generic Wayland capture is not continued; do not X11-grab there.
- P2P groups are a new L2 network; firewall rules that assume “only the AP” will break the RTP/RTSP path (a common GNOME Network Displays support issue).
- Many P2P sinks use WPS push-button; others show an 8-digit PIN on the TV. The app registers an in-process NetworkManager SecretAgent so those prompts stay in the DTK window instead of depending on nm-applet. MS-MICE with PIN off (typical Windows “Projecting to this PC” on a secure LAN) does not prompt.

### Local D-Bus (`com.ot01tool.Cast`)

The session bus is already per-user. A `.conf` in `share/dbus-1/session.d/` names the well-known name; **dbus-daemon cannot match a caller executable**. `CastDBusService` therefore resolves `GetConnectionUnixProcessID` → `/proc/<pid>/exe` and allows:

- this `ot-cast` binary (exact path)
- the DDE tray host: `/usr/bin` or `/usr/libexec` `dde-shell`, `dde-dock`, or `dde-tray-loader`

`StartScan`, `StopScan`, `Connect`, `Disconnect`, `RaiseWindow`, and `SinksJson` all go through that check. `gdbus` / `dbus-send` / a random same-user binary get `AccessDenied`. Properties (`State`, `StatusMessage`, `SelectedSinkId`) stay readable; anyone on the session bus can also subscribe to signals.

There is **no** D-Bus activation `.service` file: an untrusted caller must not be able to start `ot-cast` just by sending a method call. The tray still runs `ot-cast --background` itself.

This is not a sandbox. A process that is the tray host, that can ptrace it, or that can replace `/proc/<pid>/exe` wins. PID reuse is a TOCTOU on every Unix credential check.

### Firewall ports

Cast binds **IPv4**. A default-deny outbound-only firewall will look “fine” on the laptop and still fail on the TV.

| Path | Direction | Port / group | If blocked |
|------|-----------|--------------|------------|
| WFD RTSP | **Inbound TCP** (also outbound when the sink is P2P GO) | **7236** | Sink never starts RTSP. Windows Connect hangs after `SOURCE_READY`. |
| WFD RTP | Outbound UDP | Sink-chosen `SETUP` `client_port` (often 15550 or 1028+) | PLAY succeeds; the picture stays black. |
| MS-MICE | Outbound TCP | **7250** on the display | Same-LAN Windows Connect / Android never starts. |
| SSDP | UDP | **1900** ↔ `239.255.255.250` | No DLNA rows. |
| mDNS | UDP | **5353** ↔ `224.0.0.251` | No MS-MICE rows. |
| DLNA HTTP | **Inbound TCP** | **Ephemeral** (`QTcpServer` port 0; URI is logged) | TV `Play` fails. |
| P2P | New L2 interface | Not a TCP/UDP port | A zone that only trusts the AP drops RTSP/RTP on the group. |

### Residual risk

- `Connect` from an allowed caller **is** “share this desktop (or the chosen file) now.” MS-MICE with PIN off and DLNA do not prompt.
- RTSP :7236 and the DLNA HTTP server listen on `AnyIPv4` for the session. They can serve the live screen or a local file to whoever can reach that address on the STA LAN or the P2P group.
- No HDCP.
- Opening inbound 7236 and an ephemeral HTTP port on a hostile LAN is a trade-off for Miracast/DLNA, not a hardened service.
