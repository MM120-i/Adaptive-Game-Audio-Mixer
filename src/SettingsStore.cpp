#include "SettingsStore.h"

namespace {
    constexpr auto appFolderName = "AudioMixer";
    constexpr auto configFileName = "settings.json";
    constexpr auto logFileName = "AudioMixer.log";
}

SettingsStore::SettingsStore()
    : appDataDirectory (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile (appFolderName))
{
}

SettingsLoadResult SettingsStore::load() const {
    SettingsLoadResult result;
    const auto configFile = getConfigFile();

    if (! configFile.existsAsFile()){
        result.settings = AppSettings::createDefaults();
        result.createdDefaults = true;
        result.message = "Settings file not found; using defaults.";
        return result;
    }

    auto parsed = juce::var();
    const auto parseResult = juce::JSON::parse (configFile.loadFileAsString(), parsed);

    if (parseResult.failed()){
        result.settings = AppSettings::createDefaults();
        result.recoveredFromError = true;
        result.message = "Settings file is invalid; using defaults. " + parseResult.getErrorMessage();
        return result;
    }

    auto usedDefaults = false;
    result.settings = AppSettings::fromJson (parsed, usedDefaults);
    result.message = usedDefaults ? "Settings loaded with defaults for missing or invalid fields."
                                  : "Settings loaded.";

    return result;
}

bool SettingsStore::save (const AppSettings& settings, juce::String& errorMessage) const {
    if (!appDataDirectory.createDirectory()){
        errorMessage = "Could not create app data directory: " + appDataDirectory.getFullPathName();
        return false;
    }

    if (! getConfigFile().replaceWithText (juce::JSON::toString (settings.toJson(), true))){
        errorMessage = "Could not write settings file: " + getConfigFile().getFullPathName();
        return false;
    }

    errorMessage = {};
    return true;
}

juce::File SettingsStore::getAppDataDirectory() const{
    return appDataDirectory;
}

juce::File SettingsStore::getConfigFile() const{
    return appDataDirectory.getChildFile (configFileName);
}

juce::File SettingsStore::getLogFile() const{
    return appDataDirectory.getChildFile (logFileName);
}
