#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VolumeNotification final : public juce::TopLevelWindow, private juce::Timer {
private:
    enum class Phase { 
        FADE_IN, 
        HOLD, 
        FADE_OUT,
    };

    explicit VolumeNotification(const juce::String &);
    ~VolumeNotification() override;

    void timerCallback() override;
    void paint(juce::Graphics &) override;

    float opacity = 0.0f;
    Phase phase = Phase::FADE_IN;
    int phaseMs = 0;
    juce::String message_;

    static constexpr int fadeInMs = 150;
    static constexpr int holdMs = 1000;
    static constexpr int fadeOutMs = 400;
    static constexpr int width = 280;
    static constexpr int height = 48;

public:
    static void show(const juce::String &);
};