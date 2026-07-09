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
    return settings;
}

AppSettings AppSettings::fromJson(const juce::var &json, bool& usedDefaults) {
    auto settings = createDefaults();
    usedDefaults = false;

    const auto *object = json.getDynamicObject();

    if (object == nullptr) {
        usedDefaults = true;
        return settings;
    }

    settings.windowWidth = readBoundedInt(*object, "windowWidth", settings.windowWidth, minimumWindowWidth, maximumWindowWidth, usedDefaults);
    settings.windowHeight = readBoundedInt(*object, "windowHeight", settings.windowHeight, minimumWindowHeight, maximumWindowHeight, usedDefaults);
    settings.verboseDiagnostics = readBool(*object, "verboseDiagnostics", settings.verboseDiagnostics, usedDefaults);
    settings.lastLaunchTimestamp = readString (*object, "lastLaunchTimestamp", settings.lastLaunchTimestamp, usedDefaults);

    return settings;
}

juce::var AppSettings::toJson() const {
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("windowWidth", windowWidth);
    object->setProperty("windowHeight", windowHeight);
    object->setProperty("verboseDiagnostics", verboseDiagnostics);
    object->setProperty("lastLaunchTimestamp", lastLaunchTimestamp);

    return juce::var(object.release());
}