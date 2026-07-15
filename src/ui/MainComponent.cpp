#include "MainComponent.h"

namespace {
    constexpr int sectionPad = 20;
    constexpr int innerPad = 16;
    constexpr int controlHeight = 32;
    constexpr int gap = 14;
    constexpr float cornerRadius = 8.0f;

    const juce::Colour bgDark{ 0xff1a1d23 };
    const juce::Colour bgCard{ 0xff252830 };
    const juce::Colour accent{ 0xff6366f1 };
    const juce::Colour textPrimary{ 0xffe4e4e7 };
    const juce::Colour textSecondary{ 0xff7b7d84 };
    const juce::Colour borderSubtle{ 0xff363840 };
    const juce::Colour statusGreen{ 0xff22c55e };

    juce::Font sectionFont(){ 
        return juce::FontOptions{ 
            11.0f, 
            juce::Font::bold 
        }; 
    }

    juce::Font headerFont(){ 
        return juce::FontOptions{ 
            22.0f, 
            juce::Font::bold 
        }; 
    }

    juce::Font bodyFont(){ 
        return juce::FontOptions{ 13.0f }; 
    }

    juce::Font monoFont(){ 
        return juce::FontOptions{ 12.0f }; 
    }

    void drawCardBackground(juce::Graphics &g, juce::Rectangle<float> bounds){
        g.setColour(bgCard);
        g.fillRoundedRectangle(bounds, cornerRadius);
        g.setColour(borderSubtle);
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }

    void drawSectionHeader(juce::Graphics &g, juce::Rectangle<float> bounds, const juce::String &text){
        g.setColour(juce::Colour{ 0xff7b7d84 });
        g.setFont(sectionFont());
        g.drawText(text, bounds, juce::Justification::centredLeft);
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
        if (captureEngine.isCapturing()){
            captureEngine.stopCapture();
            startCaptureButton.setButtonText("Start Capture");
            logger.info("Capture stopped.");
        }
        else{
            auto errorMsg = juce::String();

            if (captureEngine.startCapture(errorMsg)){
                startCaptureButton.setButtonText("Stop Capture");
                logger.info ("Capture started on: " + captureEngine.getDeviceName());
                logger.info (captureEngine.getDeviceDiagnostics());
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
    diagnosticsSectionLabel.setFont(sectionFont());
    diagnosticsSectionLabel.setColour(juce::Label::textColourId, textSecondary);
    addAndMakeVisible(diagnosticsSectionLabel);

    diagnosticsEditor.setMultiLine(true);
    diagnosticsEditor.setReadOnly(true);
    diagnosticsEditor.setScrollbarsShown(true);
    diagnosticsEditor.setCaretVisible(false);
    diagnosticsEditor.setPopupMenuEnabled(true);
    diagnosticsEditor.setFont(monoFont());
    diagnosticsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour{ 0xff101418 });
    diagnosticsEditor.setColour(juce::TextEditor::textColourId, juce::Colour{ 0xffd6e2ea });
    diagnosticsEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour{ 0xff252830 });
    addAndMakeVisible(diagnosticsEditor);

    verboseDiagnosticsToggle.setToggleState(settings.verboseDiagnostics, juce::dontSendNotification);
    addAndMakeVisible(verboseDiagnosticsToggle);

    saveSettingsButton.onClick = [this] { 
        saveSettingsFromUi(); 
    };

    addAndMakeVisible(saveSettingsButton);

    reloadSettingsButton.onClick = [this] { 
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
}

void MainComponent::resized(){
    const auto bounds = getLocalBounds().reduced(sectionPad);
    auto area = bounds.toFloat();
    auto headerRow = area.removeFromTop(38.0f);

    headerLabel.setBounds(headerRow.removeFromLeft(250.0f).toNearestInt());
    versionLabel.setBounds(headerRow.toNearestInt());
    area.removeFromTop(static_cast<float>(gap));

    auto captureCard = area.removeFromTop(150.0f);
    auto cardInner = captureCard.reduced(innerPad);

    captureSectionLabel.setText("Capture", juce::dontSendNotification);
    captureSectionLabel.setBounds(cardInner.removeFromTop(18.0f).toNearestInt());
    cardInner.removeFromTop(4.0f);

    auto buttonRow = cardInner.removeFromTop(static_cast<float>(controlHeight));
    startCaptureButton.setBounds(buttonRow.removeFromLeft(140.0f).toNearestInt());
    buttonRow.removeFromLeft(12.0f);
    captureStatusLabel.setBounds(buttonRow.toNearestInt());
    cardInner.removeFromTop(static_cast<float>(gap) - 4.0f);
    deviceInfoLabel.setBounds(cardInner.removeFromTop(18.0f).toNearestInt());
    captureDetailsLabel.setBounds(cardInner.removeFromTop(16.0f).toNearestInt());
    cardInner.removeFromTop(4.0f);
    levelMeter.setBounds(cardInner.removeFromTop(40.0f).toNearestInt());
    area.removeFromTop(static_cast<float>(gap));

    auto diagCard = area.removeFromTop(area.getHeight() * 0.45f);
    auto diagInner = diagCard.reduced(innerPad);

    diagnosticsSectionLabel.setText("Diagnostics", juce::dontSendNotification);
    diagnosticsSectionLabel.setBounds(diagInner.removeFromTop(18.0f).toNearestInt());
    diagInner.removeFromTop(4.0f);
    diagnosticsEditor.setBounds(diagInner.toNearestInt());
    area.removeFromTop(static_cast<float>(gap));
    verboseDiagnosticsToggle.setBounds(area.removeFromLeft(140.0f).toNearestInt());
    area.removeFromLeft(12.0f);
    saveSettingsButton.setBounds(area.removeFromLeft(80.0f).toNearestInt());
    area.removeFromLeft(8.0f);
    reloadSettingsButton.setBounds(area.removeFromLeft(80.0f).toNearestInt());
}

void MainComponent::timerCallback(){
    if (captureEngine.isCapturing()){
        levelMeter.setLevel(captureEngine.getCurrentLevel());
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
    if (captureEngine.isCapturing()){
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
