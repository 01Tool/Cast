# Wayland / Treeland capture

A Wayland client cannot screenshot the desktop by itself. Equal-quality mirroring on DDE Wayland depends on the compositor and portal, not on DTK.

## Required path

```
xdg-desktop-portal ScreenCast → PipeWire → encoder → WFD
```

The app requests a ScreenCast session. The portal backend asks the compositor. Frames arrive as a PipeWire stream. The cast engine feeds that stream into the same encode path used on X11 (WFD RTP today; planned DLNA HTTP).

This is the same approach GNOME Network Displays uses when the mutter (or other) screencast portal is available.

## DDE status

Treeland is DDE’s wlroots-based Wayland compositor. In principle it can expose `wlr-screencopy` / `ext-image-copy-capture`. In practice the DTK app cannot invent capture:

- `xdg-desktop-portal-dde` has historically implemented **Screenshot**, not **ScreenCast**. Confirm on the target DDE version before promising Wayland.
- A different backend (`xdg-desktop-portal-wlr`, or a Treeland-specific one) could fill the gap if DDE installs and selects it.
- Talking to Treeland protocols directly from the app is less portable and should be a last resort.

Until some backend implements `org.freedesktop.portal.ScreenCast` (or an equivalent allowed capture API), Wayland mirroring cannot work.

## Invalid fallback

Grabbing via X11 APIs while `WAYLAND_DISPLAY` is set only sees **XWayland** windows. Native DDE / Qt Wayland surfaces are missing. Do not treat that as Wayland support. The engine should fail with a clear “screen capture is unavailable on this session” error.

## What the app should do now

1. Detect Wayland with `DGuiApplicationHelper::IsWaylandPlatform`.
2. Try `PortalCapture` (create ScreenCast session, attach PipeWire).
3. If the portal interface is missing or the session is denied, disable Connect and explain that this desktop session cannot share the screen.
4. Keep the same encode and transports as X11 (WFD now, DLNA later); only the frame source changes.

## Desktop-environment work (outside this app)

For Wayland to match X11, DDE needs at least one of:

- ScreenCast in `xdg-desktop-portal-dde` (preferred, native picker / permission UI)
- A shipped and selected `xdg-desktop-portal-wlr` (or Treeland equivalent) plus PipeWire
- A documented Treeland capture protocol the app is allowed to use

That work is a compositor / portal project, not a DTK widget project.
