#pragma once

#include "core/AppLogger.h"
#include "core/AppSettings.h"
#include "audio/AudioCaptureEngine.h"
#include "LevelMeter.h"
#include "core/SettingsStore.h"
#include "core/SpotifyClient.h"
#include "VolumeControl.h"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component, private juce::Timer {
public:
    MainComponent (AppSettings &, const SettingsStore &, AppLogger &);
    ~MainComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    void appendDiagnosticsMessage(const juce::String &);

    AudioCaptureEngine captureEngine;

private:
    void saveSettingsFromUi();
    void reloadSettingsIntoUi();
    void updateCaptureStatus();
    void updateSpotifyUi();
    void timerCallback() override;

    VolumeControl volumeControl;
    SpotifyClient spotifyClient;

    AppSettings &settings;
    const SettingsStore &settingsStore;
    AppLogger &logger;

    juce::Rectangle<float> captureCardRect;
    juce::Rectangle<float> volumeCardRect;
    juce::Rectangle<float> nowPlayingCardRect;
    juce::Rectangle<float> diagCardRect;

    juce::Label headerLabel;
    juce::Label versionLabel;
    juce::Label captureSectionLabel;
    juce::TextButton startCaptureButton{"Start Capture"};
    juce::Label captureStatusLabel;
    juce::Label deviceInfoLabel;
    juce::Label captureDetailsLabel;

    LevelMeter levelMeter;
    juce::Label volumeSectionLabel;
    juce::Label nowPlayingSectionLabel;
    juce::Label diagnosticsSectionLabel;
    
    juce::TextEditor diagnosticsEditor;
    juce::ToggleButton verboseDiagnosticsToggle{"Verbose logs"};
    juce::TextButton saveSettingsButton{"Save"};
    juce::TextButton reloadSettingsButton{"Reload"};
    juce::TextButton spotifyConnectButton{"Connect Spotify"};
    juce::Label spotifyStatusLabel;
    int spotifyPollCounter = 0;

    bool wasCapturing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
