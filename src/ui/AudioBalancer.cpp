#include "AudioBalancer.h"

#include <algorithm>

AudioBalancer::AudioBalancer(AudioSessionManager &mgr): sessionManager(mgr){
    initHeader();
    initGameSelector();
    initCrossfader();
    initVolumeLabels();
}

void AudioBalancer::initHeader(){
    sectionLabel.setText("Audio Mixer", juce::dontSendNotification);
    sectionLabel.setFont(juce::FontOptions{11.0f, juce::Font::bold});
    sectionLabel.setColour(juce::Label::textColourId, juce::Colour{0xff7b7d84});
    addAndMakeVisible(sectionLabel);
}

void AudioBalancer::initGameSelector(){
    gameLabel.setText("Game:", juce::dontSendNotification);
    gameLabel.setFont(juce::FontOptions{13.0f});
    gameLabel.setColour(juce::Label::textColourId, juce::Colour{0xffe4e4e7});
    addAndMakeVisible(gameLabel);

    gameDropdown.setColour(juce::ComboBox::backgroundColourId, juce::Colour{0xff101418});
    gameDropdown.setColour(juce::ComboBox::textColourId, juce::Colour{0xffe4e4e7});
    gameDropdown.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff363840});
    gameDropdown.setTextWhenNothingSelected("Select game...");
    gameDropdown.setTextWhenNoChoicesAvailable("No games detected");

    gameDropdown.onChange = [this]{
        const auto id = gameDropdown.getSelectedId();

        if(id > 0){
            selectedGamePid = id;
            selectedGameName = gameDropdown.getText();
            gameAppLabel.setText(selectedGameName, juce::dontSendNotification);
        }
            
        updateVolumes();
    };

    gameDropdown.setEnabled(true);
    addAndMakeVisible(gameDropdown);

    refreshButton.onClick = [this]{ 
        sessionManager.refreshNow();
    };
    
    addAndMakeVisible(refreshButton);
    refreshButton.setColour(juce::TextButton::buttonColourId, juce::Colour{0xff6366f1});
    refreshButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour{0xff818cf8});
    refreshButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    refreshButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void AudioBalancer::initCrossfader(){
    crossFader.setSliderStyle(juce::Slider::LinearHorizontal);
    crossFader.setRange(0.0, 1.0, 0.01);
    crossFader.setValue(0.5, juce::dontSendNotification);
    crossFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossFader.setColour(juce::Slider::trackColourId, juce::Colour{0xff6366f1});
    crossFader.setColour(juce::Slider::backgroundColourId, juce::Colour{0xff1e1b4b});

    crossFader.onValueChange = [this]{ 
        updateVolumes(); 
    };

    crossFader.setEnabled(false);
    addAndMakeVisible(crossFader);
}

void AudioBalancer::initVolumeLabels(){
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
        [](const auto &s){ 
            return s.processName.toLowerCase().contains("spotify"); 
        });

    if(spotifyIt != sessions.end()){
        newSpotifyPid = spotifyIt->pid;
        musicAppLabel.setText(spotifyIt->processName, juce::dontSendNotification);
    }

    gameDropdown.clear();

    if(selectedGamePid > 0){
        const bool stillExists = std::any_of(sessions.begin(), sessions.end(),
            [this](const auto &s){ return s.pid == selectedGamePid; });

        if(stillExists){
            gameDropdown.addItem(selectedGameName, selectedGamePid);
            gameDropdown.setSelectedId(selectedGamePid);
        }
        else {
            juce::Logger::writeToLog("Game closed: " + selectedGameName);
            selectedGamePid = 0;
            selectedGameName.clear();
            gameAppLabel.setText("", juce::dontSendNotification);
        }
    }

    if(!selectedGamePid){
        for(const auto &session : sessions){
            if(session.processName.isEmpty())
                continue;

            if(session.processName.toLowerCase().contains("spotify"))
                continue;

            gameDropdown.addItem(session.processName, session.pid);
        }
    }

    spotifyPid = newSpotifyPid;
    const bool canBalance = (spotifyPid > 0 && selectedGamePid > 0);
    crossFader.setEnabled(canBalance);

    if(selectedGamePid > 0){
        for(size_t i = 0; i < gameDropdown.getNumItems(); i++){
            if(gameDropdown.getItemId(i) == selectedGamePid){
                gameDropdown.setSelectedId(selectedGamePid);
                gameAppLabel.setText(gameDropdown.getItemText(i), juce::dontSendNotification);
                break;
            }
        }
    }

    if(canBalance)
        updateVolumes();
}

void AudioBalancer::updateVolumes(){
    if(spotifyPid <= 0 || selectedGamePid <= 0){
        crossFader.setEnabled(false);
        return;
    }

    crossFader.setEnabled(true);

    const float pos = static_cast<float>(crossFader.getValue());
    const float spotVol = 1.0f - pos;
    const float gameVol = pos;

    sessionManager.setSessionVolume(spotifyPid, spotVol);
    sessionManager.setSessionVolume(selectedGamePid, gameVol);

    musicVolLabel.setText(juce::String(static_cast<int>(spotVol * 100)) + "%", juce::dontSendNotification);
    gameVolLabel.setText(juce::String(static_cast<int>(gameVol * 100)) + "%", juce::dontSendNotification);
}

void AudioBalancer::clearSpotify(){
    if(selectedGamePid > 0){
        sessionManager.setSessionVolume(selectedGamePid, 1.0f);
        gameVolLabel.setText("100%", juce::dontSendNotification);
    }

    spotifyPid = 0;
    musicAppLabel.setText({}, juce::dontSendNotification);
    musicVolLabel.setText({}, juce::dontSendNotification);
    crossFader.setValue(1.0, juce::dontSendNotification);
    crossFader.setEnabled(false);
}

void AudioBalancer::setSystemLevel(float level){
    levelMeter.setLevel(level);
}

double AudioBalancer::getCrossFaderValue() const {
    return crossFader.getValue();
}

void AudioBalancer::setCrossFaderValue(double value){
    crossFader.setValue(value, juce::dontSendNotification);
    updateVolumes();
}

void AudioBalancer::resized(){
    const auto bounds = getLocalBounds().toFloat();

    sectionLabel.setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), 18.0f);

    auto y = sectionLabel.getBottom() + 4.0f;

    gameLabel.setBounds(bounds.getX(), y, 48.0f, 24.0f);
    const auto refreshWidth = 60.0f;
    const auto dropdownWidth = bounds.getWidth() - gameLabel.getWidth() - 8.0f - refreshWidth - 8.0f;
    gameDropdown.setBounds(gameLabel.getRight() + 8.0f, y, dropdownWidth, 24.0f);
    refreshButton.setBounds(gameDropdown.getRight() + 8.0f, y, refreshWidth, 24.0f);
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
