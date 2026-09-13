# Wayland capture

A Wayland client cannot screenshot the desktop by itself. Equal-quality mirroring depends on the compositor and portal, not on DTK.

**DDE Treeland is not this path.** Detect Treeland from `WAYLAND_DISPLAY` / `DESKTOP_SESSION` first and use [treeland.md](treeland.md) `TreelandCapture`. `DGuiApplicationHelper::IsWaylandPlatform` is true on Treeland only because DTK window chrome maps to `DTreeLandPlatformWindowInterface`. Do not select `PortalCapture` there.

## Required path

```
xdg-desktop-portal ScreenCast → PipeWire → encoder → WFD
```

The app requests a ScreenCast session. The portal backend asks the compositor. Frames arrive as a PipeWire stream. The cast engine feeds that stream into the same encode path used on X11 (WFD RTP or DLNA HTTP).

This is the same approach GNOME Network Displays uses when the mutter (or other) screencast portal is available.

## DDE vs other compositors

On a **generic** Wayland session (kwin, mutter, …), `PortalCapture` creates a ScreenCast session, lets the user pick a monitor, and opens the PipeWire remote. `GstEncoder` then runs `pipewiresrc` (package `gstreamer1.0-pipewire`). File cast still skips capture.

On **deepin V25 Treeland**, `/usr/share/xdg-desktop-portal/portals/dde.portal` lists `org.freedesktop.impl.portal.ScreenCast`, but the blocking `PortalCapture` `Start` hung the GUI after Allow. That work lives in [treeland.md](treeland.md). Cast still does not speak Treeland protocols from the app.

An **X11** session can export the ScreenCast interface with `AvailableSourceTypes = 0`. That is not Wayland support. Never fall back to X11 grab while Wayland is active.

## Invalid fallback

Grabbing via X11 APIs while `WAYLAND_DISPLAY` is set only sees **XWayland** windows. Native Qt Wayland surfaces are missing. Do not treat that as Wayland support. The engine should fail with a clear “screen capture is unavailable on this session” error.

## What the app does (generic Wayland)

1. If the session is Treeland, stop and use [treeland.md](treeland.md).
2. Else detect Wayland with `DGuiApplicationHelper::IsWaylandPlatform`.
3. `PortalCapture`: `CreateSession` → `SelectSources` (monitor + embedded cursor) → `Start` (picker) → `OpenPipeWireRemote`.
4. Copy the PipeWire fd and node onto `DisplaySource`. `GstEncoder` uses `pipewiresrc fd=3 path=<node>`, never `ximagesrc` / `x11grab`.
5. If `AvailableSourceTypes` is 0, the user cancels, or `pipewiresrc` is missing, fail with a clear error. File → DLNA/Miracast still works.

WFD LPCM on this path is video-only (the GStreamer encode path muxes AAC). AAC + Pulse/PipeWire monitor still applies when the sink lists AAC.

## Desktop-environment work (outside this app)

A working Wayland session still needs the portal backend, PipeWire, and `gstreamer1.0-pipewire`. Confirm frames on that compositor; an X11 session is not that test. Treeland confirmation is recorded in [treeland.md](treeland.md).
