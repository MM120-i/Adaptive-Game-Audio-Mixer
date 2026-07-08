# AudioMixer: Adaptive Game Music System

A Windows desktop app that dynamically blends your Spotify music with game audio, giving the impression your playlist is part of the game.

## Motivation

Many video games have soundtracks I don't enjoy or want to hear. I love hard rock and obv most games don't feature hard rock, and even if they do, some of them are just bad lmao. I'd rather have my own Spotify playlist playing in the background. But manually balancing game and Spotify volume requires alt-tabbing, exiting, and that's just annoying imo.

So I built this app to address that. It analyzes game audio in real time and automatically ducks (lowers) Spotify volume during loud moments, then brings it back during quiet moments. The result: your music feels like it belongs in the game.

Inspired by Xbox's Spotify "game vs music" volume slider, a feature Windows is missing.

## Features (MVP)

- **WASAPI Loopback**: Captures system/game audio in real time
- **DSP Analysis**: RMS, peak, transient, and silence detection
- **Dynamic Spotify Ducking**: Lowers Spotify during loud game audio, raises it during quiet
- **Spotify Integration**: OAuth PKCE, track metadata, volume control via Web API
- **Global Hotkeys**: Toggle overlay, adjust volume, toggle effects without leaving your game
- **Overlay HUD**: Always-on-top transparent window showing track info and ducking status
- **Local Presets**: Save ducking behavior per game style (Competitive, Cinematic, etc.)
- **Immersion Boost**: Intelligently raises music during quiet in-game moments for dramatic effect

## Tech Stack

- **Language:**: C++20
- **Framework:**: JUCE 8 (UI, audio, DSP)
- **Audio Capture:**: WASAPI loopback (Windows)
- **Spotify API:**: Spotify Web API (OAuth PKCE)
- **Build:**: CMake + Visual Studio 2026

## Building

```bash
# Configure and build
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug

# Run
./build/AudioMixer_artefacts/Debug/AudioMixer.exe
```
