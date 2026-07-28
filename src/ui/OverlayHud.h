#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SpotifyClient;
class AudioBalancer;

class OverlayHud final : public juce::TopLevelWindow, private juce::Timer {
private:
    void timerCallback() override;
    void paint(juce::Graphics &) override;
    void applyNativeTweaks();

    SpotifyClient &spotifyClient;
    AudioBalancer &audioBalancer;

    juce::String trackName;
    juce::String artistName;
    
    bool isPlaying = false;
    double balance = 0.5;
    bool toggleOn = false;
    int hideCounter = -1;

    static constexpr int width = 300;
    static constexpr int height = 94;
    static constexpr int refreshMs = 500;
    static constexpr int autoHideMs = 3000;

public:
    OverlayHud(SpotifyClient &, AudioBalancer &);
    ~OverlayHud() override;

    void toggle();
    void flash();
};