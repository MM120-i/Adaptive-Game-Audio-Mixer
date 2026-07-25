#include "AudioBalancer.h"

#include <algorithm>

AudioBalancer::AudioBalancer(AudioSessionManager &mgr): sessionManager(mgr){
    sectionLabel.setText("Audio Mixer", juce::dontSendNotification);
    sectionLabel.setFont(juce::FontOptions{11.0f, juce::Font::bold});
    sectionLabel.setColour(juce::Label::textColourId, juce::Colour{0xff7b7d84});
    addAndMakeVisible(sectionLabel);

    gameLabel.setText("Game:", juce::dontSendNotification);
    gameLabel.setFont(juce::FontOptions{13.0f});
    gameLabel.setColour(juce::Label::textColourId, juce::Colour{0xffe4e4e7});
    addAndMakeVisible(gameLabel);

    gameDropdown.setColour(juce::ComboBox::backgroundColourId, juce::Colour{0xff101418});
    gameDropdown.setColour(juce::ComboBox::textColourId, juce::Colour{0xffe4e4e7});
    gameDropdown.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff363840});
    
    gameDropdown.onChange = [this]{
        const auto id = gameDropdown.getSelectedId();

        if(id > 0)
            selectedGamePid = id;

        updateVolumes();
    };

    gameDropdown.setEnabled(false);
    addAndMakeVisible(gameDropdown);

    crossFader.setSliderStyle(juce::Slider::LinearHorizontal);
    crossFader.setRange(0.0, 1.0, 0.01);
    crossFader.setValue(0.5, juce::dontSendNotification);
    crossFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossFader.setColour(juce::Slider::trackColourId, juce::Colour{0xff363840});
    crossFader.setColour(juce::Slider::backgroundColourId, juce::Colour{0xff101418});

    crossFader.onValueChange = [this]{ 
        updateVolumes(); 
    };

    crossFader.setEnabled(false);
    addAndMakeVisible(crossFader);

    musicAppLabel.setFont(juce::FontOptions{11.0f});
    musicAppLabel.setColour(juce::Label::textColourId, juce::Colour{0xff7b7d84});
    musicAppLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(musicAppLabel);

    gameAppLabel.setFont(juce::FontOptions{11.0f});
    gameAppLabel.setColour(juce::Label::textColourId, juce::Colour{0xff7b7d84});
    gameAppLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gameAppLabel);

    musicVolLabel.setFont(juce::FontOptions{11.0f});
    musicVolLabel.setColour(juce::Label::textColourId, juce::Colour{0xffe4e4e7});
    musicVolLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(musicVolLabel);

    gameVolLabel.setFont(juce::FontOptions{11.0f});
    gameVolLabel.setColour(juce::Label::textColourId, juce::Colour{0xffe4e4e7});
    gameVolLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gameVolLabel);

    addAndMakeVisible(levelMeter);
}

void AudioBalancer::refreshSessions(){
    auto sessions = sessionManager.getActiveSessions();
    int newSpotifyPid = 0;

    auto spotifyIt = std::find_if(sessions.begin(), sessions.end(),
        [](const auto &s){ return s.processName.toLowerCase().contains("spotify"); });

    if(spotifyIt != sessions.end()){
        newSpotifyPid = spotifyIt->pid;
        musicAppLabel.setText(spotifyIt->processName, juce::dontSendNotification);
    }

    gameDropdown.clear();
    int itemId = 1;

    for(const auto &session : sessions){
        if(session.pid == newSpotifyPid)
            continue;

        gameDropdown.addItem(session.processName, session.pid);
        itemId++;
    }

    if(selectedGamePid > 0){
        for(size_t i = 0; i < gameDropdown.getNumItems(); i++){
            if(gameDropdown.getItemId(i) == selectedGamePid){
                gameDropdown.setSelectedId(selectedGamePid);
                gameAppLabel.setText(gameDropdown.getItemText(i), juce::dontSendNotification);
                break;
            }
        }
    }

    spotifyPid = newSpotifyPid;
    const bool canBalance = (spotifyPid > 0 && selectedGamePid > 0);
    crossFader.setEnabled(canBalance);
    gameDropdown.setEnabled(gameDropdown.getNumItems() > 0);

    if(canBalance)
        updateVolumes();
}

void AudioBalancer::updateVolumes(){
    if(spotifyPid <= 0 || selectedGamePid <= 0)
        return;

    const float pos = static_cast<float>(crossFader.getValue());
    const float spotVol = 1.0f - pos;
    const float gameVol = pos;

    sessionManager.setSessionVolume(spotifyPid, spotVol);
    sessionManager.setSessionVolume(selectedGamePid, gameVol);

    musicVolLabel.setText(juce::String(static_cast<int>(spotVol * 100)) + "%", juce::dontSendNotification);
    gameVolLabel.setText(juce::String(static_cast<int>(gameVol * 100)) + "%", juce::dontSendNotification);
}

void AudioBalancer::setSystemLevel(float level){
    levelMeter.setLevel(level);
}

void AudioBalancer::resized(){
    const auto bounds = getLocalBounds().toFloat();

    sectionLabel.setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), 18.0f);

    auto y = sectionLabel.getBottom() + 4.0f;

    gameLabel.setBounds(bounds.getX(), y, 48.0f, 24.0f);
    gameDropdown.setBounds(gameLabel.getRight() + 8.0f, y, bounds.getWidth() - 56.0f, 24.0f);
    y = gameDropdown.getBottom() + 14.0f;

    const float sliderHeight = 24.0f;
    crossFader.setBounds(bounds.getX(), y, bounds.getWidth(), sliderHeight);
    y = crossFader.getBottom() + 2.0f;

    musicAppLabel.setBounds(bounds.getX(), y, 60.0f, 14.0f);
    gameAppLabel.setBounds(bounds.getRight() - 60.0f, y, 60.0f, 14.0f);
    y = musicAppLabel.getBottom() + 1.0f;

    musicVolLabel.setBounds(bounds.getX(), y, 60.0f, 14.0f);
    gameVolLabel.setBounds(bounds.getRight() - 60.0f, y, 60.0f, 14.0f);
    y = musicVolLabel.getBottom() + 6.0f;

    levelMeter.setBounds(bounds.getX(), y, bounds.getWidth(), bounds.getBottom() - y);
}