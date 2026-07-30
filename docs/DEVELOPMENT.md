# Development

## Coding conventions

- C++20, JUCE 8, Windows only
- `void foo(){...}` — no space before parentheses
- `static constexpr` for compile-time constants
- `std::atomic` for thread-safe primitives
- `juce::CriticalSection` / `std::mutex` for locks
- `std::unique_ptr` for ownership, raw pointers only for non-owning references
- COM objects wrapped in RAII (`ComPtr`, `TaskMemPtr`)
- `#define NOMINMAX` before Windows.h to prevent `min`/`max` macro clashes

## Project structure

```
src/
├── audio/
│   ├── AudioCaptureEngine.h/.cpp   # WASAPI loopback capture
│   └── AudioSessionManager.h/.cpp  # COM per-app session enumeration + volume
├── core/
│   ├── AppLogger.h/.cpp            # File-based logger
│   ├── AppSettings.h/.cpp          # JSON settings serialization
│   ├── GlobalHotkeys.h/.cpp        # WH_KEYBOARD_LL global hook
│   ├── SettingsStore.h/.cpp        # Atomic JSON I/O + backup recovery
│   ├── SpotifyClient.h/.cpp        # OAuth PKCE + Web API
│   └── SystemTray.h/.cpp           # Shell_NotifyIcon tray
├── ui/
│   ├── AudioBalancer.h/.cpp        # Game selector + crossfader + meter
│   ├── LevelMeter.h/.cpp           # RMS-to-dB meter with peak hold
│   ├── MainComponent.h/.cpp        # 4-card main layout
│   ├── MixerLookAndFeel.h          # JUCE colour scheme
│   ├── OverlayHud.h/.cpp           # Click-through transparent overlay
│   ├── VolumeControl.h/.cpp        # Slider + mute with debounce
│   └── VolumeNotification.h/.cpp   # Self-deleting fade popup
└── Main.cpp                        # JUCEApplication entry point
```

## Running tests

```bash
.\build.bat test       # Build + run all 60 tests
.\test.ps1            # Alternative: verbose test output
```

Tests use JUCE's `UnitTest` framework with `beginTest()`, `expect()`, and `expectEquals()`.

## Adding a test

1. Create `tests/YourTest.cpp`
2. Define a class inheriting `juce::UnitTest`
3. Add `runTest()` with `beginTest(...)` / `expect(...)` blocks
4. Create a static instance (auto-registers with JUCE)
5. Add the file to `target_sources(AudioMixerTests ...)` in `CMakeLists.txt`
6. Add any needed source dependencies to the test target

## Linting

```bash
.\build.bat lint
```

Runs both MSVC `/analyze` and `cppcheck` with exhaustive checks.

## Common Windows APIs used

| API | Where | Purpose |
|-----|-------|---------|
| `IAudioSessionManager2` | `AudioSessionManager` | Per-app volume control |
| `ISimpleAudioVolume` | `AudioSessionManager` | Set session volume |
| `IAudioClient` | `AudioCaptureEngine` | WASAPI loopback capture |
| `SetWindowsHookEx(WH_KEYBOARD_LL)` | `GlobalHotkeys` | Global keyboard hook |
| `Shell_NotifyIcon` | `SystemTray` | Tray icon + menu |
| `ActivateAudioInterfaceAsync` | (attempted, dropped) | Per-process loopback |
