#pragma once

#include <juce_core/juce_core.h>

struct AppSettings{
    int windowWidth { 900 };
    int windowHeight { 560 };
    bool verboseDiagnostics { true };
    juce::String lastLaunchTimestamp;

    static AppSettings createDefaults();
    static AppSettings fromJson (const juce::var& json, bool& usedDefaults);

    juce::var toJson() const;
};
