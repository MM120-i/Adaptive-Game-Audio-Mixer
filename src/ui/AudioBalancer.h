#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "audio/AudioSessionManager.h"
#include "LevelMeter.h"

class AudioBalancer final : public juce::Component {
private:
    void updateVolumes();

    AudioSessionManager &sessionManager;

    juce::Label sectionLabel;
    juce::Label gameLabel;
    juce::ComboBox gameDropdown;
    juce::TextButton refreshButton{"Refresh"};
    juce::Slider crossFader;
    juce::Label musicAppLabel;
    juce::Label gameAppLabel;
    juce::Label musicVolLabel;
    juce::Label gameVolLabel;

    LevelMeter levelMeter;

    int spotifyPid = 0;
    int selectedGamePid = 0;
    juce::String selectedGameName;

public:
    explicit AudioBalancer(AudioSessionManager &);

    void refreshSessions();
    void setSystemLevel(float);
    void resized() override;
};