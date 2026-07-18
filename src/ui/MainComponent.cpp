#include "MainComponent.h"

namespace {
    constexpr int sectionPad = 20;
    constexpr int innerPad = 14;
    constexpr int controlHeight = 32;
    constexpr int gap = 12;
    constexpr float cornerRadius = 8.0f;

    const juce::Colour bgDark{0xff1a1d23};
    const juce::Colour bgCard{0xff252830};
    const juce::Colour accent{0xff6366f1};
    const juce::Colour textPrimary{0xffe4e4e7};
    const juce::Colour textSecondary{0xff7b7d84};
    const juce::Colour borderSubtle{0xff363840};
    const juce::Colour statusGreen{0xff22c55e};

    juce::Font sectionFont(){
        return juce::FontOptions{11.0f, juce::Font::bold};
    }

    juce::Font headerFont(){
        return juce::FontOptions{22.0f, juce::Font::bold};
    }

    juce::Font bodyFont(){
        return juce::FontOptions{13.0f};
    }

    juce::Font monoFont(){
        return juce::FontOptions{12.0f};
    }

    void drawCard(juce::Graphics &g, juce::Rectangle<float> r){
        g.setColour(bgCard);
        g.fillRoundedRectangle(r, cornerRadius);
        g.setColour(borderSubtle);
        g.drawRoundedRectangle(r, cornerRadius, 1.0f);
    }
}

MainComponent::MainComponent(AppSettings &appSettings, const SettingsStore &store, AppLogger &appLogger)
    : settings(appSettings),
      settingsStore(store),
      logger(appLogger)
{
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

    captureSectionLabel.setFont(sectionFont());
    captureSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(captureSectionLabel);

    startCaptureButton.onClick = [this]{
        if(captureEngine.isCapturing()){
            captureEngine.stopCapture();
            startCaptureButton.setButtonText("Start Capture");
            logger.info("Capture stopped.");
        }
        else{
            auto errorMsg = juce::String();
            if(captureEngine.startCapture(errorMsg)){
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

    captureStatusLabel.setFont(bodyFont());
    captureStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(captureStatusLabel);

    deviceInfoLabel.setFont(bodyFont());
    deviceInfoLabel.setColour(juce::Label::textColourId, textPrimary);
    deviceInfoLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(deviceInfoLabel);

    captureDetailsLabel.setFont(bodyFont());
    captureDetailsLabel.setColour(juce::Label::textColourId, textSecondary);
    captureDetailsLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(captureDetailsLabel);

    addAndMakeVisible(levelMeter);

    volumeSectionLabel.setFont(sectionFont());
    volumeSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(volumeSectionLabel);

    volumeControl.setCommitCallback([this](int volumePercent){
        logger.info("Volume set to: " + juce::String(volumePercent) + "%");
    });

    addAndMakeVisible(volumeControl);

    nowPlayingSectionLabel.setFont(sectionFont());
    nowPlayingSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(nowPlayingSectionLabel);

    nowPlayingLabel.setFont(bodyFont());
    nowPlayingLabel.setColour(juce::Label::textColourId, textSecondary);
    nowPlayingLabel.setJustificationType(juce::Justification::centredLeft);
    nowPlayingLabel.setText("No track connected", juce::dontSendNotification);
    addAndMakeVisible(nowPlayingLabel);

    diagnosticsSectionLabel.setFont(sectionFont());
    diagnosticsSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(diagnosticsSectionLabel);

    diagnosticsEditor.setMultiLine(true);
    diagnosticsEditor.setReadOnly(true);
    diagnosticsEditor.setScrollbarsShown(true);
    diagnosticsEditor.setCaretVisible(false);
    diagnosticsEditor.setPopupMenuEnabled(true);
    diagnosticsEditor.setFont(monoFont());
    diagnosticsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour{0xff101418});
    diagnosticsEditor.setColour(juce::TextEditor::textColourId, juce::Colour{0xffd6e2ea});
    diagnosticsEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff252830});
    addAndMakeVisible(diagnosticsEditor);

    verboseDiagnosticsToggle.setToggleState(settings.verboseDiagnostics, juce::dontSendNotification);
    addAndMakeVisible(verboseDiagnosticsToggle);

    saveSettingsButton.onClick = [this]{ 
        saveSettingsFromUi(); 
    };

    addAndMakeVisible(saveSettingsButton);

    reloadSettingsButton.onClick = [this]{ 
        reloadSettingsIntoUi(); 
    };

    addAndMakeVisible(reloadSettingsButton);

    updateCaptureStatus();
    startTimerHz(20);
    logger.info("UI created.");
}

MainComponent::~MainComponent() = default;

void MainComponent::paint(juce::Graphics &g){
    g.fillAll(bgDark);
    drawCard(g, captureCardRect);
    drawCard(g, volumeCardRect);
    drawCard(g, nowPlayingCardRect);
    drawCard(g, diagCardRect);
}

void MainComponent::resized(){
    const auto bounds = getLocalBounds().reduced(sectionPad);
    auto area = bounds.toFloat();
    auto headerRow = area.removeFromTop(38.0f);

    headerLabel.setBounds(headerRow.removeFromLeft(250.0f).toNearestInt());
    versionLabel.setBounds(headerRow.toNearestInt());
    area.removeFromTop(static_cast<float>(gap));

    const auto captureWidth = area.getWidth() * 0.56f;
    const auto volumeWidth  = area.getWidth() - captureWidth - static_cast<float>(gap);
    auto row = area.removeFromTop(195.0f);

    captureCardRect = row.removeFromLeft(captureWidth);
    auto capInner = captureCardRect.reduced(innerPad);

    captureSectionLabel.setText("Capture", juce::dontSendNotification);
    captureSectionLabel.setBounds(capInner.removeFromTop(18.0f).toNearestInt());
    capInner.removeFromTop(4.0f);

    auto buttonRow = capInner.removeFromTop(static_cast<float>(controlHeight));
    startCaptureButton.setBounds(buttonRow.removeFromLeft(140.0f).toNearestInt());
    buttonRow.removeFromLeft(10.0f);
    captureStatusLabel.setBounds(buttonRow.toNearestInt());

    capInner.removeFromTop(6.0f);
    deviceInfoLabel.setBounds(capInner.removeFromTop(18.0f).toNearestInt());
    captureDetailsLabel.setBounds(capInner.removeFromTop(16.0f).toNearestInt());
    capInner.removeFromTop(6.0f);
    levelMeter.setBounds(capInner.toNearestInt());

    row.removeFromLeft(static_cast<float>(gap));
    volumeCardRect = row;
    auto volInner = volumeCardRect.reduced(innerPad);

    volumeSectionLabel.setText("Volume", juce::dontSendNotification);
    volumeSectionLabel.setBounds(volInner.removeFromTop(18.0f).toNearestInt());
    volInner.removeFromTop(6.0f);
    volumeControl.setBounds(volInner.toNearestInt());
    area.removeFromTop(static_cast<float>(gap));
    nowPlayingCardRect = area.removeFromTop(44.0f);

    auto npInner = nowPlayingCardRect.reduced(innerPad);

    nowPlayingSectionLabel.setText("Now Playing", juce::dontSendNotification);
    nowPlayingSectionLabel.setBounds(npInner.removeFromTop(18.0f).toNearestInt());
    nowPlayingLabel.setBounds(npInner.toNearestInt());

    area.removeFromTop(static_cast<float>(gap));
    diagCardRect = area;

    auto diagInner = diagCardRect.reduced(innerPad);

    diagnosticsSectionLabel.setText("Diagnostics", juce::dontSendNotification);
    diagnosticsSectionLabel.setBounds(diagInner.removeFromTop(18.0f).toNearestInt());
    diagInner.removeFromTop(4.0f);

    const auto editorHeight = diagInner.getHeight() - 32.0f;
    diagnosticsEditor.setBounds(diagInner.removeFromTop(editorHeight).toNearestInt());
    diagInner.removeFromTop(6.0f);

    auto settingsRow = diagInner;
    verboseDiagnosticsToggle.setBounds(settingsRow.removeFromLeft(140.0f).toNearestInt());
    settingsRow.removeFromLeft(8.0f);
    saveSettingsButton.setBounds(settingsRow.removeFromLeft(80.0f).toNearestInt());
    settingsRow.removeFromLeft(8.0f);
    reloadSettingsButton.setBounds(settingsRow.removeFromLeft(80.0f).toNearestInt());
}

void MainComponent::timerCallback(){
    const auto capturing = captureEngine.isCapturing();
    levelMeter.setLevel(capturing ? captureEngine.getCurrentLevel() : 0.0f);

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

void MainComponent::appendDiagnosticsMessage(const juce::String &message){
    diagnosticsEditor.moveCaretToEnd();
    diagnosticsEditor.insertTextAtCaret(message + juce::newLine);
}

void MainComponent::saveSettingsFromUi(){
    settings.verboseDiagnostics = verboseDiagnosticsToggle.getToggleState();
    settings.lastLaunchTimestamp = juce::Time::getCurrentTime().toISO8601(true);

    auto errorMessage = juce::String();

    if(settingsStore.save(settings, errorMessage))
        logger.info("Settings saved.");
    else
        logger.error("Settings save failed: " + errorMessage);
}

void MainComponent::reloadSettingsIntoUi(){
    const auto loadResult = settingsStore.load();
    settings = loadResult.settings;
    verboseDiagnosticsToggle.setToggleState(settings.verboseDiagnostics, juce::dontSendNotification);

    if(loadResult.recoveredFromError)
        logger.error(loadResult.message);
    else
        logger.info(loadResult.message);
}
