#include <juce_core/juce_core.h>
#include "core/AppSettings.h"
#include "core/SettingsStore.h"

class AppSettingsTests final : public juce::UnitTest {
public:
    AppSettingsTests() : juce::UnitTest("AppSettings", "Phase 1"){}

    void runTest() override{
        beginTest("createDefaults --- produces sensible default values");
        {
            const auto defaults = AppSettings::createDefaults();

            expectGreaterOrEqual(defaults.windowWidth, 640);
            expectGreaterOrEqual(defaults.windowHeight, 420);
            expect(defaults.verboseDiagnostics == true);
            expect(defaults.lastLaunchTimestamp.isNotEmpty());
        }

        beginTest("toJson -> fromJson --- round-trip preserves all values");
        {
            AppSettings original;
            original.windowWidth = 1200;
            original.windowHeight = 800;
            original.verboseDiagnostics = false;

            bool usedDefaults = false;
            const auto roundTripped = AppSettings::fromJson(original.toJson(), usedDefaults);

            expect(usedDefaults == false);
            expectEquals(roundTripped.windowWidth,  original.windowWidth);
            expectEquals(roundTripped.windowHeight, original.windowHeight);
            expect(roundTripped.verboseDiagnostics == original.verboseDiagnostics);
        }

        beginTest("fromJson --- missing keys fall back to defaults");
        {
            auto *obj = new juce::DynamicObject();
            obj->setProperty ("windowWidth", 1280);  

            juce::var json(obj);
            bool usedDefaults = false;
            const auto settings = AppSettings::fromJson(json, usedDefaults);

            expectEquals(settings.windowWidth, 1280);   
            expect(usedDefaults == true);              
        }

        beginTest("fromJson --- invalid json structure uses defaults");
        {
            juce::var json = juce::var("garbage");

            bool usedDefaults = false;
            const auto settings = AppSettings::fromJson(json, usedDefaults);

            expect(usedDefaults == true);  
        }

        beginTest("fromJson --- out-of-range values clamped to defaults");
        {
            auto *obj = new juce::DynamicObject();
            obj->setProperty("windowWidth", 100); 

            juce::var json(obj);
            bool usedDefaults = false;
            const auto settings = AppSettings::fromJson(json, usedDefaults);

            expect(usedDefaults == true); 
            expectGreaterOrEqual(settings.windowWidth, 640); 
        }

        beginTest("volume presets --- round-trip preserves all values");
        {
            AppSettings original;
            original.volumePresets.clear();
            original.volumePresets.add({"Test Preset", 42});

            bool usedDefaults = false;
            const auto roundTripped = AppSettings::fromJson(original.toJson(), usedDefaults);

            expect(usedDefaults == false);
            expectEquals(roundTripped.volumePresets.size(), 1);
            expectEquals(roundTripped.volumePresets[0].name, juce::String("Test Preset"));
            expectEquals(roundTripped.volumePresets[0].volume, 42);
        }

        beginTest("volume presets --- defaults contain four presets");
        {
            const auto defaults = AppSettings::createDefaults();
            expectEquals(defaults.volumePresets.size(), 4);
            expectEquals(defaults.volumePresets[0].name, juce::String("Game Focus"));
            expectEquals(defaults.volumePresets[0].volume, 30);
            expectEquals(defaults.volumePresets[3].name, juce::String("Full Send"));
            expectEquals(defaults.volumePresets[3].volume, 100);
        }
    }
};

static AppSettingsTests appSettingsTests;
