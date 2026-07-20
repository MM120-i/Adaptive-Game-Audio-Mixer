#pragma once

#include <juce_core/juce_core.h>

struct AppSettings{
    int windowWidth{ 2000 };
    int windowHeight{ 2000 };
    juce::int64 spotifyTokenExpiry = 0;
    bool verboseDiagnostics{ true };

    juce::String lastLaunchTimestamp;
    juce::String spotifyRefreshToken;
    juce::String spotifyAccessToken;

    static AppSettings createDefaults();
    static AppSettings fromJson(const juce::var &, bool &);

    juce::var toJson() const;
};
