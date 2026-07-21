#pragma once

#include <juce_core/juce_core.h>

struct VolumePreset {
    juce::String name;
    int volume;
};

struct AppSettings {
    int windowWidth{ 2000 };
    int windowHeight{ 2000 };
    juce::int64 spotifyTokenExpiry = 0;
    bool verboseDiagnostics{ true };

    juce::String lastLaunchTimestamp;
    juce::String spotifyRefreshToken;
    juce::String spotifyAccessToken;
    juce::Array<VolumePreset> volumePresets;
    int defaultPresetIndex = 0;

    static AppSettings createDefaults();
    static AppSettings fromJson(const juce::var &, bool &);

    juce::var toJson() const;
};
