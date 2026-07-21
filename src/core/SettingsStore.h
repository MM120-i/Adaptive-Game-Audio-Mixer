#pragma once

#include "AppSettings.h"

#include <juce_core/juce_core.h>

struct SettingsLoadResult {
    AppSettings settings;
    bool createdDefaults{ false };
    bool recoveredFromError{ false };
    juce::String message;
};

class SettingsStore {
public:
    SettingsStore();

    SettingsLoadResult load() const;
    bool save (const AppSettings &, juce::String &) const;

    juce::File getConfigFile() const;
    juce::File getLogFile() const;

private:
    juce::File appDataDirectory;
};
