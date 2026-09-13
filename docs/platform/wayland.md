# Wayland capture

**Not continued.** DDE sessions are **Treeland or X11**. Generic Wayland (mutter, kwin, …) is not a current product path. `PortalCapture` stays in the tree (`src/capture/portalcapture.cpp`) so a later mutter/kwin pass does not rebuild the ScreenCast client. `CastEngine` does **not** construct it.

A Wayland client cannot screenshot the desktop by itself. Equal-quality mirroring depends on the compositor and portal, not on DTK.

**DDE Treeland is not this path.** Detect Treeland from `WAYLAND_DISPLAY` / `DESKTOP_SESSION` first and use [treeland.md](treeland.md) `TreelandCapture`. `DGuiApplicationHelper::IsWaylandPlatform` is true on Treeland only because DTK window chrome maps to `DTreeLandPlatformWindowInterface`. Do not select `PortalCapture` there. Do not fold Treeland into this class.

## Required path

```
xdg-desktop-portal ScreenCast → PipeWire → encoder → WFD
```

The app requests a ScreenCast session. The portal backend asks the compositor. Frames arrive as a PipeWire stream. The cast engine feeds that stream into the same encode path used on X11 (WFD RTP or DLNA HTTP).

This is the same approach GNOME Network Displays uses when the mutter (or other) screencast portal is available.

## DDE vs other compositors

On a **generic** Wayland session (kwin, mutter, …), the engine sets `DisplayServer::Wayland`, leaves `m_capture` empty, and fails screen mirror with a clear error. File cast still skips capture and still works. Do not X11-grab.

The parked `PortalCapture` class is the mutter-style blocking ScreenCast client (`CreateSession` → `SelectSources` → `Start` → `OpenPipeWireRemote`). It is unmeasured on mutter/kwin. Do not call it until this path is continued.

On **deepin V25 Treeland**, `/usr/share/xdg-desktop-portal/portals/dde.portal` lists `org.freedesktop.impl.portal.ScreenCast`, but the blocking `PortalCapture` `Start` hung the GUI after Allow. That work lives in [treeland.md](treeland.md). Cast still does not speak Treeland protocols from the app.

An **X11** session can export the ScreenCast interface with `AvailableSourceTypes = 0`. That is not Wayland support. Never fall back to X11 grab while Wayland is active.

## Invalid fallback

Grabbing via X11 APIs while `WAYLAND_DISPLAY` is set only sees **XWayland** windows. Native Qt Wayland surfaces are missing. Do not treat that as Wayland support. The engine should fail with a clear “generic Wayland capture is not continued” error.

## What the app does (generic Wayland)

1. If the session is Treeland, stop and use [treeland.md](treeland.md).
2. Else detect Wayland with `DGuiApplicationHelper::IsWaylandPlatform`.
3. Do **not** construct `PortalCapture`. Do **not** X11-grab. Fail screen mirror: capture on generic Wayland is not continued.
4. File → DLNA/Miracast still works (no capture).
5. Keep `PortalCapture` compiled. A later pass would: `CreateSession` → `SelectSources` → `Start` → `OpenPipeWireRemote`, then `pipewiresrc` (never `ximagesrc` / `x11grab`).

WFD LPCM on the parked GStreamer path is video-only (that encode path muxes AAC). Not a current measurement.

## Desktop-environment work (outside this app)

A working generic Wayland session would still need the portal backend, PipeWire, and `gstreamer1.0-pipewire`. That is not a current test. Treeland confirmation is recorded in [treeland.md](treeland.md).
