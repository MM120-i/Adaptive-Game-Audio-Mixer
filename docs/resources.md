WASAPI Resources:
- Endpoints, IAudioClient, capture/render, and audio sessions
https://learn.microsoft.com/en-us/windows/win32/coreaudio/wasapi

- WASAPI Loopback captures the audio being played by a render endpoint, this acc matches our MVP
https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording

- Read entire thing on IAudioCaptureClient
https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudiocaptureclient

- Maybe for later reading on Windows audio sessions instead of Spotify API
https://learn.microsoft.com/en-us/windows/win32/coreaudio/audio-sessions





JUCE Docs:
- Focusing on GUI Basics, AudioDeviceManager, audio buffer handling, sliders/meters, and JSON
https://juce.com/learn/tutorials/

- Intro to DSP
https://juce.com/learn/tutorials/




DSP Fundamentals:
Big ass textbook, focusing on:
- Sampling
- RMS / Signal amplitude
- Moving averages
- Filters
- Fourier basics (a lot acc)

https://www.dspguide.com/




Spotify API:
- Auth Code with PKCE
Looking at:
    - Code verifier
    - Code challenge
    - localhost redirect
    - refresh tokens
https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow

- Set Playback Volume (Requires Premium)
https://developer.spotify.com/documentation/web-api/reference/set-volume-for-users-playback

- Rate limits
https://developer.spotify.com/documentation/web-api/concepts/rate-limits





CMake:
https://cmake.org/cmake/help/latest/guide/tutorial/index.html




Others:
- real-time audio programming C++ avoid allocation
- audio callback thread safety
- envelope follower attack release audio
- audio ducking algorithm attack release hysteresis