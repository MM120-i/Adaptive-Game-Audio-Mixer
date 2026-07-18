#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class VolumeControl final : public juce::Component, private juce::Timer {
public:
    using CommitCallback = std::function<void(int volumePercent)>;

    VolumeControl();

    void setCommitCallback(CommitCallback cb) {
        commitCallback = std::move(cb);
    }

    void setVolume(int);
    int getVolume() const { return currentVolume; }
    bool isMuted() const { return muted; }

    void resized() override;
    void paint(juce::Graphics &) override;

private:
    void timerCallback() override;

    juce::Slider volumeSlider;
    juce::TextButton muteButton{"M"};
    juce::Label volumeLabel;

    int currentVolume = 65;
    int preMuteVolume = 65;
    int pendingVolume = 65;
    int lastCommited = -1;
    bool muted = false;

    CommitCallback commitCallback;

    static constexpr int debounceMs = 200;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeControl);
};
