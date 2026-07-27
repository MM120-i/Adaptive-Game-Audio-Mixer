#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "core/AppLogger.h"
#include "core/AppSettings.h"
#include "audio/AudioCaptureEngine.h"
#include "LevelMeter.h"
#include "core/SettingsStore.h"
#include "core/SpotifyClient.h"
#include "VolumeControl.h"
#include "audio/AudioSessionManager.h"
#include "ui/AudioBalancer.h"

class MainComponent final : public juce::Component, private juce::Timer {
private:
    void updateCaptureStatus();
    void updateSpotifyUi();
    void timerCallback() override;

    void initHeader();
    void initCaptureSection();
    void initVolumeSection();
    void initSpotifySection();
    void initSessionMonitor();

    void layoutHeader(juce::Rectangle<float> &area);
    void layoutMixerCard(const juce::Rectangle<float> &card);
    void layoutVolumeCard(const juce::Rectangle<float> &card);
    void layoutNowPlayingCard(juce::Rectangle<float> &area);
    void layoutSystemOutputCard(juce::Rectangle<float> &area);

    AppSettings &settings;
    const SettingsStore &settingsStore;
    AppLogger &logger;

    juce::Rectangle<float> mixerCardRect;
    juce::Rectangle<float> volumeCardRect;
    juce::Rectangle<float> nowPlayingCardRect;
    juce::Rectangle<float> systemOutputCardRect;

    juce::Label headerLabel;
    juce::Label versionLabel;
    juce::Label systemOutputSectionLabel;
    juce::TextButton startCaptureButton{"Start Capture"};
    juce::Label captureStatusLabel;
    juce::Label deviceInfoLabel;
    juce::Label captureDetailsLabel;

    LevelMeter levelMeter;
    juce::Label volumeSectionLabel;
    juce::Label nowPlayingSectionLabel;

    juce::TextButton spotifyConnectButton{"Connect Spotify"};
    juce::TextButton prevButton{juce::String::fromUTF8("\xe2\x8f\xae")};
    juce::TextButton playPauseButton{juce::String::fromUTF8("\xe2\x96\xb6")};
    juce::TextButton nextButton{juce::String::fromUTF8("\xe2\x8f\xad")};
    juce::Label spotifyStatusLabel;

    int spotifyPollCounter = 0;
    bool wasCapturing = false;
    bool volumeChanging_ = false;
    SpotifyStatus lastSpotifyStatus = SpotifyStatus::Disconnected;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)

public:
    MainComponent (AppSettings &, const SettingsStore &, AppLogger &);
    ~MainComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    VolumeControl volumeControl;
    SpotifyClient spotifyClient;

    AudioCaptureEngine captureEngine;
    AudioSessionManager sessionManager;
    AudioBalancer audioBalancer{sessionManager};
};
