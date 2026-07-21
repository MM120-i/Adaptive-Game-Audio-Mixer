#include "VolumeControl.h"

#include <algorithm>

namespace {
    constexpr int buttonSize = 28;
    constexpr int labelWidth = 52;
    constexpr int rowGap = 4;

    const juce::Colour accentColour{0xff6366f1};
    const juce::Colour mutedColour{0xffef4444};
}

VolumeControl::VolumeControl(){
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setRange(0.0, 100.0, 1.0);
    volumeSlider.setValue(static_cast<double>(currentVolume), juce::dontSendNotification);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour{0xff363840});
    volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour{0xff101418});

    volumeSlider.onValueChange = [this] {
        pendingVolume = static_cast<int>(volumeSlider.getValue());
        volumeLabel.setText(juce::String(pendingVolume) + "%", juce::dontSendNotification);

        if(muted){
            muted = false;
            muteButton.setColour(juce::TextButton::buttonColourId, accentColour);
        }

        startTimer(debounceMs);
    };

    addAndMakeVisible(volumeSlider);

    volumeLabel.setFont(juce::FontOptions{24.0f, juce::Font::bold});
    volumeLabel.setJustificationType(juce::Justification::centredRight);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colour{0xffe4e4e7});
    volumeLabel.setText(juce::String(currentVolume) + "%", juce::dontSendNotification);
    addAndMakeVisible(volumeLabel);

    muteButton.setColour(juce::TextButton::buttonColourId, accentColour);

    muteButton.onClick = [this] {
        muted = !muted;

        if(muted){
            preMuteVolume = currentVolume;
            volumeSlider.setValue(0.0, juce::dontSendNotification);
            volumeLabel.setText("0%", juce::dontSendNotification);
            currentVolume = 0;
            muteButton.setColour(juce::TextButton::buttonColourId, mutedColour);
        }
        else{
            currentVolume = preMuteVolume;
            volumeSlider.setValue(static_cast<double>(currentVolume), juce::dontSendNotification);
            volumeLabel.setText(juce::String(currentVolume) + "%", juce::dontSendNotification);
            muteButton.setColour(juce::TextButton::buttonColourId, accentColour);
        }

        if(commitCallback)
            commitCallback(currentVolume);
    };

    addAndMakeVisible(muteButton);
}

void VolumeControl::setVolume(int percent){
    currentVolume = juce::jlimit(0, 100, percent);
    pendingVolume = currentVolume;
    lastCommited = currentVolume;

    volumeSlider.setValue(static_cast<double>(currentVolume), juce::dontSendNotification);
    volumeLabel.setText(juce::String(currentVolume) + "%", juce::dontSendNotification);
}

// TODO: refactor - Large if block
void VolumeControl::timerCallback(){
    if(animating_){
        animProgress_ += 16.0f / static_cast<float>(animDuration_);

        if(animProgress_ >= 1.0f){
            animProgress_ = 1.0f;
            animating_ = false;
            stopTimer();
        }

        float t = animProgress_;
        float eased = (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t);
        float value = animStart_ + (animTarget_ - animStart_) * eased;
        int intVal = static_cast<int>(value);

        currentVolume = intVal;
        volumeSlider.setValue(value, juce::dontSendNotification);
        volumeLabel.setText(juce::String(intVal) + "%", juce::dontSendNotification);

        if(!animating_){
            lastCommited = currentVolume;

            if(commitCallback)
                commitCallback(currentVolume);

            pendingVolume = currentVolume;
        }

        return;
    }

    stopTimer();

    if(pendingVolume == lastCommited)
        return;

    currentVolume = pendingVolume;
    lastCommited = pendingVolume;

    if(commitCallback)
        commitCallback(currentVolume);
}

void VolumeControl::resized(){
    const auto bounds = getLocalBounds().toFloat();
    const auto labelWidthF = static_cast<float>(labelWidth);
    const auto sliderW = bounds.getWidth() - labelWidthF - 8.0f;

    volumeSlider.setBounds(
        static_cast<int>(bounds.getX()),
        static_cast<int>(bounds.getY()),
        static_cast<int>(sliderW),
        24
    );

    volumeLabel.setBounds(
        static_cast<int>(bounds.getRight() - labelWidthF),
        static_cast<int>(bounds.getY()),
        labelWidth,
        24
    );

    const auto centerX = bounds.getCentreX();
    const auto buttonY = bounds.getY() + 24.0f + 4.0f;

    muteButton.setBounds(
        static_cast<int>(centerX - buttonSize * 0.5f),
        static_cast<int>(buttonY),
        buttonSize,
        buttonSize
    );
}

void VolumeControl::paint(juce::Graphics &g){
    g.fillAll(juce::Colour{0xff1a1d23});
}

void VolumeControl::animateToVolume(int target, int durationMs){
    if(durationMs <= 0){
        setVolume(target);
        return;
    }

    stopTimer();

    const auto clampedTarget = std::clamp(target, 0, 100);

    if(muted && clampedTarget > 0){
        muted = false;
        preMuteVolume = clampedTarget;
        muteButton.setColour(juce::TextButton::buttonColourId, accentColour);
    }

    animStart_ = static_cast<float>(currentVolume);
    animTarget_ = static_cast<float>(clampedTarget);
    animProgress_ = 0.0f;
    animDuration_ = durationMs;
    animating_ = true;
    startTimer(16);
}