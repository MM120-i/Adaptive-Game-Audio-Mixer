#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LevelMeter final : public juce::Component {
private:
    float level = 0.0f;
    float decay = 0.0f;
    float peakDb = -60.0f;

    static constexpr float decayRate = 0.92f;
    static constexpr float peakHoldMs = 1500.0f;

    double peakHoldStart = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)

public:
    LevelMeter() = default;

    void setLevel(float);
    void paint(juce::Graphics &) override;
};
