# Architecture

## Overview

AudioMixer is a Windows desktop app that lets you blend Spotify audio with any other running `.exe` using a crossfader.

```
┌───────────────────────────────────────────────────────┐
│  Main.cpp — Application entry point                   │
│  ┌─────────────┐  ┌──────────┐  ┌─────────────────┐   │
│  │ Hotkeys     │  │ Tray     │  │ OverlayHud      │   │
│  │(WH_KEYBOARD │  │(Shell_   │  │(WS_EX_LAYERED + │   │
│  │ _LL hook)   │  │NotifyIcon)│  │ WS_EX_TRANSPARENT) │
│  └─────────────┘  └──────────┘  └─────────────────┘   │
│                                                       │
│  ┌───────────────────────────────────────────────┐    │
│  │  MainComponent — 4-card layout                │    │
│  │  ┌──────────────┐  ┌───────────┐              │    │
│  │  │ AudioBalancer│  │ VolumeCtrl│              │    │
│  │  │ (crossfader) │  │ (slider)  │              │    │
│  │  └──────────────┘  └───────────┘              │    │
│  │  ┌──────────────┐  ┌───────────────┐          │    │
│  │  │ Now Playing  │  │ System Output │          │    │
│  │  │ (Spotify UI) │  │ (capture+dev) │          │    │
│  │  └──────────────┘  └───────────────┘          │    │
│  └───────────────────────────────────────────────┘    │
└───────────────────────────────────────────────────────┘
```

## Data flow

```
Spotify API -> SpotifyClient -> VolumeControl (slider) -> audioBalancer
                                                                  │
WASAPI loopback -> AudioCaptureEngine -> MainComponent (timer)   ─┤
                                                                  │
IAudioSessionManager2 -> AudioSessionManager -> AudioBalancer   ──┤
                                                                  │
                                              ┌──── per-app ──────┘
                                              │    volume
                                              ▼
                                     ISimpleAudioVolume
                                     SetMasterVolume(pid, level)
```

## Key components

| Component             | File         | Purpose                                                          |
| --------------------- | ------------ | ---------------------------------------------------------------- |
| `AudioSessionManager` | `src/audio/` | Enumerates audio sessions via COM, controls per-app volume       |
| `AudioCaptureEngine`  | `src/audio/` | WASAPI shared-mode loopback capture, RMS level computation       |
| `AudioBalancer`       | `src/ui/`    | Game selector dropdown, crossfader, system level meter           |
| `VolumeControl`       | `src/ui/`    | Horizontal volume slider, mute toggle, debounced commits         |
| `SpotifyClient`       | `src/core/`  | OAuth PKCE auth, token refresh, Web API (play/pause/skip/volume) |
| `GlobalHotkeyManager` | `src/core/`  | `SetWindowsHookEx(WH_KEYBOARD_LL)` global keyboard hook          |
| `SystemTray`          | `src/core/`  | `Shell_NotifyIcon` tray icon with right-click menu               |
| `OverlayHud`          | `src/ui/`    | Click-through `TopLevelWindow` with `WS_EX_LAYERED`              |
| `AppSettings`         | `src/core/`  | JSON settings with type-validated deserialization + backup       |
| `SettingsStore`       | `src/core/`  | Atomic file writes with automatic `.bak` recovery                |

## Threads

| Thread                    | Purpose                                                  |
| ------------------------- | -------------------------------------------------------- |
| **Main (JUCE message)**   | UI rendering, event handling                             |
| **Hotkey message pump**   | `SetWindowsHookEx` keyboard processing                   |
| **Tray message pump**     | `Shell_NotifyIcon` notification handling                 |
| **WASAPI capture**        | Loopback audio reading at 10ms intervals                 |
| **Audio session monitor** | COM enumeration every 1s (or on-demand via `refreshNow`) |
| **Spotify poll**          | Track info and device state every 3s                     |
