#pragma once

#include "AppLogger.h"
#include "AppSettings.h"
#include "SettingsStore.h"
#include "LevelMeter.h"
#include "AudioCaptureEngine.h"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component, private juce::Timer {
public:
    MainComponent (AppSettings &, const SettingsStore &, AppLogger &);
    ~MainComponent() override;

    void paint (juce::Graphics &) override;
    void resized() override;
    void appendDiagnosticsMessage(const juce::String &);
    void updateDeviceInfo();
    void timerCallback() override;

private:
    void saveSettingsFromUi();
    void reloadSettingsIntoUi();
    void refreshSettingsSummary();

    AppSettings &settings;
    const SettingsStore &settingsStore;
    AppLogger &logger;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::Label settingsSummaryLabel;
    juce::ToggleButton verboseDiagnosticsToggle;
    juce::TextButton saveSettingsButton{ "Save Settings" };
    juce::TextButton reloadSettingsButton{ "Reload Settings" };
    juce::TextEditor diagnosticsEditor;
    
    AudioCaptureEngine captureEngine;
    LevelMeter levelMeter;

    juce::Label deviceInfoLabel;
    juce::Label sampleRateLabel;
    juce::Label channelsLabel;
    juce::Label bufferLabel;
    juce::TextButton startCaptureButton{"Start Capture"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
