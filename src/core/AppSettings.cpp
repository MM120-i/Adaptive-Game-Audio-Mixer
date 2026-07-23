#include "AppSettings.h"

namespace {
    constexpr int minimumWindowWidth = 640;
    constexpr int minimumWindowHeight = 420;
    constexpr int maximumWindowWidth = 3840;
    constexpr int maximumWindowHeight = 2160;

    int readBoundedInt (const juce::DynamicObject &object, const juce::Identifier &key, int fallback, int minimum, int maximum, bool &usedDefaults) {
        if (!object.hasProperty(key)){
            usedDefaults = true;
            return fallback;
        }

        const auto value = static_cast<int>(object.getProperty(key));

        if (value < minimum || value > maximum){
            usedDefaults = true;
            return fallback;
        }

        return value;
    }

    bool readBool(const juce::DynamicObject &object, const juce::Identifier &key, bool fallback, bool &usedDefaults){
        if (!object.hasProperty(key)){
            usedDefaults = true;
            return fallback;
        }

        return static_cast<bool>(object.getProperty(key));
    }

    juce::String readString(const juce::DynamicObject &object, const juce::Identifier &key, const juce::String &fallback, bool &usedDefaults){
        if (!object.hasProperty(key)){
            usedDefaults = true;
            return fallback;
        }

        return object.getProperty(key).toString();
    }
}

AppSettings AppSettings::createDefaults() {
    AppSettings settings;
    settings.lastLaunchTimestamp = juce::Time::getCurrentTime().toISO8601(true);

    settings.volumePresets.add({"Game Focus", 30});
    settings.volumePresets.add({"Balanced", 60});
    settings.volumePresets.add({"Music Focus", 85});
    settings.volumePresets.add({"Full Send", 100});

    return settings;
}

AppSettings AppSettings::fromJson(const juce::var &json, bool& usedDefaults) {
    auto settings = createDefaults();
    usedDefaults = false;
    const auto *object = json.getDynamicObject();

    if (!object) {
        usedDefaults = true;
        return settings;
    }

    settings.windowWidth = readBoundedInt(
        *object, 
        "windowWidth", 
        settings.windowWidth, 
        minimumWindowWidth, 
        maximumWindowWidth, 
        usedDefaults
    );

    settings.windowHeight = readBoundedInt(
        *object, 
        "windowHeight", 
        settings.windowHeight, 
        minimumWindowHeight, 
        maximumWindowHeight, 
        usedDefaults
    );

    settings.verboseDiagnostics = readBool(
        *object, 
        "verboseDiagnostics", 
        settings.verboseDiagnostics, 
        usedDefaults
    );

    settings.lastLaunchTimestamp = readString(
        *object, 
        "lastLaunchTimestamp", 
        settings.lastLaunchTimestamp, 
        usedDefaults
    );

    settings.spotifyAccessToken = readString(
        *object, 
        "spotifyAccessToken",  
        {}, 
        usedDefaults
    );

    settings.spotifyRefreshToken = readString(
        *object, 
        "spotifyRefreshToken", 
        {}, 
        usedDefaults
    );

    if (object->hasProperty("spotifyTokenExpiry")){
        settings.spotifyTokenExpiry = static_cast<juce::int64>(object->getProperty("spotifyTokenExpiry"));
    }
    else {
        settings.spotifyTokenExpiry = 0;
        usedDefaults = true;
    }

    if(object->hasProperty("volumePresets")){
        const auto arr = object->getProperty("volumePresets");

        if(arr.isArray()){
            settings.volumePresets.clear();

            for(const auto &item : *arr.getArray())
                settings.volumePresets.add({
                    item.getProperty("name", juce::var()).toString(),
                    static_cast<int>(item.getProperty("volume", juce::var()))
                });
        }
        else {
            usedDefaults = true;
        }
    }
    else {
        usedDefaults = true;
    }

    if(object->hasProperty("defaultPresetIndex"))
        settings.defaultPresetIndex = static_cast<int>(object->getProperty("defaultPresetIndex"));

    if(object->hasProperty("runAtStartup"))
        settings.runAtStartup = static_cast<bool>(object->getProperty("runAtStartup"));

    if(object->hasProperty("minimizeToTray"))
        settings.minimizeToTray = static_cast<bool>(object->getProperty("minimizeToTray"));

    return settings;
}

juce::var AppSettings::toJson() const {
    auto object = std::make_unique<juce::DynamicObject>();
    
    object->setProperty("windowWidth", windowWidth);
    object->setProperty("windowHeight", windowHeight);
    object->setProperty("verboseDiagnostics", verboseDiagnostics);
    object->setProperty("lastLaunchTimestamp", lastLaunchTimestamp);
    object->setProperty("spotifyAccessToken", spotifyAccessToken);
    object->setProperty("spotifyRefreshToken", spotifyRefreshToken);
    object->setProperty("spotifyTokenExpiry", spotifyTokenExpiry);

    juce::Array<juce::var> presetVars;

    for(const auto &preset : volumePresets){
        auto *obj = new juce::DynamicObject();
        obj->setProperty("name", preset.name);
        obj->setProperty("volume", preset.volume);
        presetVars.add(juce::var(obj));
    }

    object->setProperty("volumePresets", presetVars);
    object->setProperty("defaultPresetIndex", defaultPresetIndex);
    object->setProperty("runAtStartup", runAtStartup);
    object->setProperty("minimizeToTray", minimizeToTray);

    return juce::var(object.release());
}