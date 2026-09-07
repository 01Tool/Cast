# Wayland / Treeland capture

A Wayland client cannot screenshot the desktop by itself. Equal-quality mirroring on DDE Wayland depends on the compositor and portal, not on DTK.

## Required path

```
xdg-desktop-portal ScreenCast → PipeWire → encoder → WFD
```

The app requests a ScreenCast session. The portal backend asks the compositor. Frames arrive as a PipeWire stream. The cast engine feeds that stream into the same encode path used on X11 (WFD RTP or DLNA HTTP).

This is the same approach GNOME Network Displays uses when the mutter (or other) screencast portal is available.

## DDE status (deepin V25)

Treeland is DDE’s wlroots-based Wayland compositor. **Cast does not speak Treeland protocols.** It uses `org.freedesktop.portal.ScreenCast`; `xdg-desktop-portal-dde` talks to Treeland (`treeland_capture_*`, `zwlr_screencopy`, PipeWire) behind that portal.

On V25, `/usr/share/xdg-desktop-portal/portals/dde.portal` lists `org.freedesktop.impl.portal.ScreenCast`. `PortalCapture` creates a ScreenCast session, lets the user pick a monitor, and opens the PipeWire remote. `GstEncoder` then runs `pipewiresrc` (package `gstreamer1.0-pipewire`). File cast still skips capture.

An **X11** session can export the ScreenCast interface with `AvailableSourceTypes = 0`. That is not Treeland support. Never fall back to X11 grab while Wayland is active.

Talking to `treeland-capture` from `ot-cast` stays a last resort.

## Invalid fallback

Grabbing via X11 APIs while `WAYLAND_DISPLAY` is set only sees **XWayland** windows. Native DDE / Qt Wayland surfaces are missing. Do not treat that as Wayland support. The engine should fail with a clear “screen capture is unavailable on this session” error.

## What the app does

1. Detect Wayland with `DGuiApplicationHelper::IsWaylandPlatform`.
2. `PortalCapture`: `CreateSession` → `SelectSources` (monitor + embedded cursor) → `Start` (picker) → `OpenPipeWireRemote`.
3. Copy the PipeWire fd and node onto `DisplaySource`. `GstEncoder` uses `pipewiresrc fd=3 path=<node>`, never `ximagesrc` / `x11grab`.
4. If `AvailableSourceTypes` is 0, the user cancels, or `pipewiresrc` is missing, fail with a clear error. File → DLNA/Miracast still works.

WFD LPCM on this path is video-only (the GStreamer encode path muxes AAC). AAC + Pulse/PipeWire monitor still applies when the sink lists AAC.

## Desktop-environment work (outside this app)

A working Treeland session still needs the portal backend, PipeWire, and `gstreamer1.0-pipewire`. Confirm frames on a **Treeland** login; an X11 session is not that test.
