#pragma once

#include "core/AppLogger.h"
#include "core/AppSettings.h"
#include "audio/AudioCaptureEngine.h"
#include "LevelMeter.h"
#include "core/SettingsStore.h"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component, private juce::Timer {
public:
    MainComponent (AppSettings &, const SettingsStore &, AppLogger &);
    ~MainComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    void appendDiagnosticsMessage(const juce::String &);

private:
    void saveSettingsFromUi();
    void reloadSettingsIntoUi();
    void updateCaptureStatus();
    void timerCallback() override;

    AppSettings &settings;
    const SettingsStore &settingsStore;
    AppLogger &logger;

    juce::Label headerLabel;
    juce::Label versionLabel;
    juce::Label captureSectionLabel;
    juce::TextButton startCaptureButton{ "Start Capture" };
    juce::Label captureStatusLabel;
    juce::Label deviceInfoLabel;
    juce::Label captureDetailsLabel;

    AudioCaptureEngine captureEngine;
    LevelMeter levelMeter;

    juce::Label diagnosticsSectionLabel;
    juce::TextEditor diagnosticsEditor;

    juce::ToggleButton verboseDiagnosticsToggle{ "Verbose logs" };
    juce::TextButton saveSettingsButton{ "Save" };
    juce::TextButton reloadSettingsButton{ "Reload" };

    bool wasCapturing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
