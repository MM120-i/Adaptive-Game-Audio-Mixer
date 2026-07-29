#include "MainComponent.h"

namespace {
    constexpr int sectionPad = 20;
    constexpr int innerPad = 14;
    constexpr int controlHeight = 30;
    constexpr int gap = 12;
    constexpr float cornerRadius = 12.0f;

    const juce::Colour bgDark{0xff0a0a14};
    const juce::Colour bgCard{0xff12121f};
    const juce::Colour accent{0xff06b6d4};
    const juce::Colour accentDim{0xff0891b2};
    const juce::Colour accentGlow{0xff00f0ff};
    const juce::Colour textPrimary{0xfff1f5f9};
    const juce::Colour textSecondary{0xff94a3b8};
    const juce::Colour borderSubtle{0xff1e293b};
    const juce::Colour statusGreen{0xff10ff7e};

    juce::Font sectionFont(){
        return juce::FontOptions{11.0f, juce::Font::bold};
    }

    juce::Font headerFont(){
        return juce::FontOptions{22.0f, juce::Font::bold};
    }

    juce::Font bodyFont(){
        return juce::FontOptions{13.0f};
    }

    void drawCard(juce::Graphics &g, juce::Rectangle<float> r){
        juce::DropShadow shadow{juce::Colours::black.withAlpha(0.4f), 8, {0, 2}};
        shadow.drawForRectangle(g, r.toNearestInt());

        g.setColour(bgCard);
        g.fillRoundedRectangle(r, cornerRadius);
        g.setColour(borderSubtle);
        g.drawRoundedRectangle(r, cornerRadius, 0.5f);
    }

    void styleButton(juce::TextButton &btn){
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour{0xff6366f1});
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour{0xff818cf8});
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        btn.setLookAndFeel(nullptr);
    }
}

MainComponent::MainComponent(AppSettings &appSettings, const SettingsStore &store, AppLogger &appLogger)
    : settings(appSettings),
      settingsStore(store),
      logger(appLogger)
{
    initHeader();
    initCaptureSection();
    initVolumeSection();
    initSpotifySection();
    initSessionMonitor();

    spotifyClient.loadTokens(settings);
    spotifyClient.startPolling();
    updateSpotifyUi();
    updateCaptureStatus();
    addAndMakeVisible(audioBalancer);
    startTimerHz(20);
}

void MainComponent::initHeader(){
    headerLabel.setText("AudioMixer", juce::dontSendNotification);
    headerLabel.setFont(headerFont());
    headerLabel.setColour(juce::Label::textColourId, accent);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(headerLabel);

    versionLabel.setText("v" + juce::String(AUDIO_MIXER_VERSION), juce::dontSendNotification);
    versionLabel.setFont(bodyFont());
    versionLabel.setColour(juce::Label::textColourId, textSecondary);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(versionLabel);
}

void MainComponent::initCaptureSection(){
    systemOutputSectionLabel.setFont(sectionFont());
    systemOutputSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(systemOutputSectionLabel);

    startCaptureButton.onClick = [this]{
        if(captureEngine.isCapturing()){
            captureEngine.stopCapture();
            startCaptureButton.setButtonText("Start Capture");
            logger.info("Capture stopped.");
        }
        else{
            auto errorMsg = juce::String();
            juce::String deviceId;
            auto selId = outputDeviceDropdown.getSelectedId();

            if(selId > 1 && static_cast<size_t>(selId - 2) < deviceList_.size())
                deviceId = deviceList_[selId - 2].second;

            if(captureEngine.startCapture(errorMsg, deviceId)){
                startCaptureButton.setButtonText("Stop Capture");
                logger.info("Capture started on: " + captureEngine.getDeviceName());
                logger.info(captureEngine.getDeviceDiagnostics());
            }
            else{
                logger.error("Capture failed: " + errorMsg);
            }
        }

        updateCaptureStatus();
    };

    addAndMakeVisible(startCaptureButton);
    styleButton(startCaptureButton);

    captureStatusLabel.setFont(bodyFont());
    captureStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(captureStatusLabel);

    outputDeviceDropdown.setColour(juce::ComboBox::backgroundColourId, juce::Colour{0xff101418});
    outputDeviceDropdown.setColour(juce::ComboBox::textColourId, juce::Colour{0xffe4e4e7});
    outputDeviceDropdown.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff363840});
    outputDeviceDropdown.setTextWhenNothingSelected("Default output device");
    outputDeviceDropdown.setTextWhenNoChoicesAvailable("No devices found");
    addAndMakeVisible(outputDeviceDropdown);

    refreshDevicesButton.onClick = [this]{
        populateDeviceDropdown();
    };
    addAndMakeVisible(refreshDevicesButton);
    styleButton(refreshDevicesButton);

    populateDeviceDropdown();

    deviceInfoLabel.setFont(bodyFont());
    deviceInfoLabel.setColour(juce::Label::textColourId, textPrimary);
    deviceInfoLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(deviceInfoLabel);

    captureDetailsLabel.setFont(bodyFont());
    captureDetailsLabel.setColour(juce::Label::textColourId, textSecondary);
    captureDetailsLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(captureDetailsLabel);
}

void MainComponent::initVolumeSection(){
    volumeSectionLabel.setFont(sectionFont());
    volumeSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(volumeSectionLabel);

    volumeControl.setCommitCallback([this](int volumePercent){
        volumeChanging_ = true;
        spotifyClient.setVolume(volumePercent);
        volumeChanging_ = false;
    });

    addAndMakeVisible(volumeControl);
}

void MainComponent::initSpotifySection(){
    spotifyConnectButton.onClick = [this]{
        if(spotifyClient.isAuthenticated())
            spotifyClient.disconnect();
        else
            spotifyClient.startAuth();

        updateSpotifyUi();
    };

    addAndMakeVisible(spotifyConnectButton);
    styleButton(spotifyConnectButton);

    spotifyStatusLabel.setFont(bodyFont());
    spotifyStatusLabel.setColour(juce::Label::textColourId, textSecondary);
    spotifyStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(spotifyStatusLabel);

    spotifyClient.onStateChanged = [this]{
        const auto newStatus = spotifyClient.status();

        if(newStatus != lastSpotifyStatus){
            lastSpotifyStatus = newStatus;
            spotifyClient.saveTokens(settings);

            if(newStatus == SpotifyStatus::Connected)
                spotifyClient.fetchDeviceVolume();
            else if(newStatus == SpotifyStatus::Disconnected)
                audioBalancer.clearSpotify();
        }

        updateSpotifyUi();
    };

    prevButton.onClick = [this]{ spotifyClient.skipPrevious(); };
    addAndMakeVisible(prevButton);
    styleButton(prevButton);

    playPauseButton.onClick = [this]{ spotifyClient.setPlaying(!spotifyClient.isPlaying()); };
    addAndMakeVisible(playPauseButton);
    styleButton(playPauseButton);

    nextButton.onClick = [this]{ spotifyClient.skipNext(); };
    addAndMakeVisible(nextButton);
    styleButton(nextButton);

    nowPlayingSectionLabel.setFont(sectionFont());
    nowPlayingSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(nowPlayingSectionLabel);
}

void MainComponent::initSessionMonitor(){
    sessionManager.onSessionChanged = [this]{
        juce::MessageManager::callAsync([this]{
            audioBalancer.refreshSessions();
        });
    };
    
    sessionManager.start();
}

MainComponent::~MainComponent() = default;

void MainComponent::paint(juce::Graphics &g){
    g.fillAll(bgDark);
    drawCard(g, mixerCardRect);
    drawCard(g, volumeCardRect);
    drawCard(g, nowPlayingCardRect);
    drawCard(g, systemOutputCardRect);
}

void MainComponent::resized(){
    const auto bounds = getLocalBounds().reduced(sectionPad);
    auto area = bounds.toFloat();

    layoutHeader(area);
    area.removeFromTop(static_cast<float>(gap));

    const auto mixerWidth = area.getWidth() * 0.60f;
    const auto volumeWidth = area.getWidth() - mixerWidth - static_cast<float>(gap);
    auto row = area.removeFromTop(280.0f);

    mixerCardRect = row.removeFromLeft(mixerWidth);
    layoutMixerCard(mixerCardRect);

    row.removeFromLeft(static_cast<float>(gap));
    volumeCardRect = row;
    layoutVolumeCard(volumeCardRect);

    area.removeFromTop(static_cast<float>(gap));
    layoutNowPlayingCard(area);

    area.removeFromTop(static_cast<float>(gap));
    layoutSystemOutputCard(area);
}

void MainComponent::layoutHeader(juce::Rectangle<float> &area){
    auto headerRow = area.removeFromTop(38.0f);
    headerLabel.setBounds(headerRow.removeFromLeft(250.0f).toNearestInt());
    versionLabel.setBounds(headerRow.toNearestInt());
}

void MainComponent::layoutMixerCard(const juce::Rectangle<float> &card){
    audioBalancer.setBounds(card.reduced(innerPad).toNearestInt());
}

void MainComponent::layoutVolumeCard(const juce::Rectangle<float> &card){
    auto inner = card.reduced(innerPad);

    volumeSectionLabel.setText("Volume", juce::dontSendNotification);
    volumeSectionLabel.setBounds(inner.removeFromTop(18.0f).toNearestInt());
    inner.removeFromTop(6.0f);

    auto vcHeight = 64.0f;
    auto vcY = inner.getY() + (inner.getHeight() - vcHeight) * 0.5f;
    volumeControl.setBounds(inner.withY(vcY).withHeight(vcHeight).toNearestInt());
}

void MainComponent::layoutNowPlayingCard(juce::Rectangle<float> &area){
    nowPlayingCardRect = area.removeFromTop(140.0f);
    auto inner = nowPlayingCardRect.reduced(innerPad);

    nowPlayingSectionLabel.setText("Now Playing", juce::dontSendNotification);
    nowPlayingSectionLabel.setBounds(inner.removeFromTop(18.0f).toNearestInt());
    inner.removeFromTop(4.0f);

    auto spotifyRow = inner.removeFromTop(static_cast<float>(controlHeight));
    spotifyConnectButton.setBounds(spotifyRow.removeFromLeft(140.0f).toNearestInt());
    spotifyRow.removeFromLeft(10.0f);
    spotifyStatusLabel.setBounds(spotifyRow.toNearestInt());
    inner.removeFromTop(6.0f);

    auto transportRow = inner.removeFromTop(static_cast<float>(controlHeight));
    prevButton.setBounds(transportRow.removeFromLeft(60.0f).toNearestInt());
    transportRow.removeFromLeft(8.0f);
    playPauseButton.setBounds(transportRow.removeFromLeft(60.0f).toNearestInt());
    transportRow.removeFromLeft(8.0f);
    nextButton.setBounds(transportRow.removeFromLeft(60.0f).toNearestInt());
}

void MainComponent::layoutSystemOutputCard(juce::Rectangle<float> &area){
    systemOutputCardRect = area.removeFromTop(150.0f);
    auto inner = systemOutputCardRect.reduced(innerPad);

    systemOutputSectionLabel.setText("System Output", juce::dontSendNotification);
    systemOutputSectionLabel.setBounds(inner.removeFromTop(18.0f).toNearestInt());
    inner.removeFromTop(4.0f);

    auto buttonRow = inner.removeFromTop(static_cast<float>(controlHeight));
    outputDeviceDropdown.setBounds(buttonRow.removeFromLeft(inner.getWidth() * 0.55f).toNearestInt());
    buttonRow.removeFromLeft(6.0f);
    refreshDevicesButton.setBounds(buttonRow.removeFromLeft(70.0f).toNearestInt());
    buttonRow.removeFromLeft(6.0f);
    startCaptureButton.setBounds(buttonRow.removeFromLeft(140.0f).toNearestInt());
    buttonRow.removeFromLeft(10.0f);
    captureStatusLabel.setBounds(buttonRow.toNearestInt());

    inner.removeFromTop(6.0f);
    deviceInfoLabel.setBounds(inner.removeFromTop(18.0f).toNearestInt());
    captureDetailsLabel.setBounds(inner.removeFromTop(16.0f).toNearestInt());
}
void MainComponent::populateDeviceDropdown(){
    auto savedId = outputDeviceDropdown.getSelectedId();
    outputDeviceDropdown.clear();
    outputDeviceDropdown.addItem("Default", 1);

    deviceList_ = AudioCaptureEngine::enumerateRenderDevices();

    for(size_t i = 0; i < deviceList_.size(); i++)
        outputDeviceDropdown.addItem(deviceList_[i].first, static_cast<int>(i + 2));

    outputDeviceDropdown.setSelectedId(savedId > 0 ? savedId : 1, juce::dontSendNotification);
}

void MainComponent::timerCallback(){
    const auto capturing = captureEngine.isCapturing();
    const auto level = capturing ? captureEngine.getCurrentLevel() : 0.0f;

    audioBalancer.setSystemLevel(level);

    if(wasCapturing && !capturing && captureEngine.getCaptureError().isNotEmpty()){
        logger.error("Capture stopped unexpectedly: " + captureEngine.getCaptureError());
        updateCaptureStatus();
    }

    wasCapturing = capturing;

    if(capturing){
        deviceInfoLabel.setText("Device: " + captureEngine.getDeviceName(), juce::dontSendNotification);
        captureDetailsLabel.setText(
            juce::String(captureEngine.getSampleRate()) + " Hz  |  "
            + juce::String(captureEngine.getChannelCount()) + " ch  |  "
            + juce::String(captureEngine.getBufferSize()) + " samples",
            juce::dontSendNotification
        );
    }
    else {
        deviceInfoLabel.setText({}, juce::dontSendNotification);
        captureDetailsLabel.setText({}, juce::dontSendNotification);
    }

    spotifyPollCounter++;

    if(spotifyPollCounter >= 40){
        spotifyPollCounter = 0;
        updateSpotifyUi();
    }
}

void MainComponent::updateCaptureStatus(){
    if(captureEngine.isCapturing()){
        captureStatusLabel.setColour(juce::Label::textColourId, statusGreen);
        captureStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f Active"), juce::dontSendNotification);
    }
    else{
        captureStatusLabel.setColour(juce::Label::textColourId, textSecondary);
        captureStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f Idle"), juce::dontSendNotification);
    }
}

void MainComponent::updateSpotifyUi(){
    if(!volumeChanging_ && !volumeControl.isMuted()){
        const int spotVol = spotifyClient.deviceVolume();

        if(spotVol != volumeControl.getVolume())
            volumeControl.setVolume(spotVol);
    }

    volumeControl.setControlsEnabled(spotifyClient.isAuthenticated() && spotifyClient.hasActiveDevice());

    playPauseButton.setButtonText(spotifyClient.isPlaying()
        ? juce::String::fromUTF8("\xe2\x9d\x9a\xe2\x9d\x9a")
        : juce::String::fromUTF8("\xe2\x96\xb6\xef\xb8\x8e"));
 
    switch (spotifyClient.status()){
        case SpotifyStatus::Error:
            spotifyConnectButton.setButtonText("Connect Spotify");
            spotifyStatusLabel.setText(spotifyClient.lastErrorMessage(), juce::dontSendNotification);
            return;

        case SpotifyStatus::Connecting:
            spotifyConnectButton.setButtonText("Connect Spotify");
            spotifyStatusLabel.setText("Connecting... check your browser", juce::dontSendNotification);
            return;
    }

    if(spotifyClient.isAuthenticated()){
        spotifyConnectButton.setButtonText("Disconnect");

        switch(spotifyClient.status()){
            case SpotifyStatus::Connected:
                if(spotifyClient.trackTitle().isNotEmpty())
                    spotifyStatusLabel.setText(spotifyClient.trackTitle() + " - " + spotifyClient.trackArtist(), juce::dontSendNotification);
                else
                    spotifyStatusLabel.setText("Connected — waiting for track info", juce::dontSendNotification);
                
                break;

            case SpotifyStatus::NoActiveDevice:
                spotifyStatusLabel.setText("Connected — No active device. Open Spotify.", juce::dontSendNotification);
                break;

            default:
                spotifyStatusLabel.setText("Connected", juce::dontSendNotification);
                break;
        }
    }
    else {
        spotifyConnectButton.setButtonText("Connect Spotify");
        spotifyStatusLabel.setText("Not connected", juce::dontSendNotification);
    }
}
