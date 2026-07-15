#include "MainComponent.h"

namespace {
    constexpr int padding = 20;
    constexpr int controlHeight = 30;
}

MainComponent::MainComponent(AppSettings &appSettings, const SettingsStore &store, AppLogger &appLogger): 
    settings(appSettings),
    settingsStore(store),
    logger(appLogger)
{
    titleLabel.setText("AudioMixer", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions{ 26.0f, juce::Font::bold });
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    versionLabel.setText("Version " + juce::String (AUDIO_MIXER_VERSION), juce::dontSendNotification);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(versionLabel);

    settingsSummaryLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(settingsSummaryLabel);

    verboseDiagnosticsToggle.setButtonText("Verbose diagnostics");
    verboseDiagnosticsToggle.setToggleState(settings.verboseDiagnostics, juce::dontSendNotification);

    verboseDiagnosticsToggle.onClick = [this] {
        settings.verboseDiagnostics = verboseDiagnosticsToggle.getToggleState();
        refreshSettingsSummary();
    };

    addAndMakeVisible(verboseDiagnosticsToggle);

    saveSettingsButton.onClick = [this] { 
        saveSettingsFromUi(); 
    };

    addAndMakeVisible(saveSettingsButton);

    reloadSettingsButton.onClick = [this] { 
        reloadSettingsIntoUi(); 
    };

    addAndMakeVisible (reloadSettingsButton);

    deviceInfoLabel.setJustificationType(juce::Justification::centredLeft);

    diagnosticsEditor.setMultiLine(true);
    diagnosticsEditor.setReadOnly(true);
    diagnosticsEditor.setScrollbarsShown(true);
    diagnosticsEditor.setCaretVisible(false);
    diagnosticsEditor.setPopupMenuEnabled(true);
    diagnosticsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour{ 0xff101418 });
    diagnosticsEditor.setColour(juce::TextEditor::textColourId, juce::Colour{ 0xffd6e2ea });
    
    addAndMakeVisible(diagnosticsEditor);
    addAndMakeVisible(deviceInfoLabel);
    addAndMakeVisible(sampleRateLabel);
    addAndMakeVisible(channelsLabel);;
    addAndMakeVisible(bufferLabel);

    startCaptureButton.onClick = [this]{
        if(captureEngine.isCapturing()){
            captureEngine.stopCapture();
            startCaptureButton.setButtonText("Start Capture");
            logger.info("Capture stopped");
        }
        else{
            auto errorMessage = juce::String();

            if(captureEngine.startCapture(errorMessage)){
                startCaptureButton.setButtonText("Stop Capture");
                logger.info("Capture started on: " + captureEngine.getDeviceName());
            }
            else{
                logger.error("Capture failed: " + errorMessage);
            }
        }
    };

    addAndMakeVisible(startCaptureButton);
    addAndMakeVisible(levelMeter);

    startTimer(20);

    refreshSettingsSummary();
    logger.info("UI created.");
}

MainComponent::~MainComponent() = default;

void MainComponent::paint (juce::Graphics &graphics) {
    graphics.fillAll(juce::Colour{ 0xfff6f7f8 });
}

void MainComponent::resized() {
    auto bounds = getLocalBounds().reduced(padding);
    auto header = bounds.removeFromTop(42);
    titleLabel.setBounds(header.removeFromLeft(300));
    versionLabel.setBounds(header);

    bounds.removeFromTop(8);
    settingsSummaryLabel.setBounds(bounds.removeFromTop(controlHeight));
    bounds.removeFromTop(8);

    auto controls = bounds.removeFromTop(controlHeight);
    verboseDiagnosticsToggle.setBounds(controls.removeFromLeft(220));
    controls.removeFromLeft(8);
    saveSettingsButton.setBounds(controls.removeFromLeft(140));
    controls.removeFromLeft(8);
    reloadSettingsButton.setBounds(controls.removeFromLeft(150));

    bounds.removeFromTop(14);
    diagnosticsEditor.setBounds(bounds);

    bounds.removeFromTop(4);
    startCaptureButton.setBounds(bounds.removeFromTop(controlHeight).removeFromLeft(150));
    bounds.removeFromTop(8);

    auto infoRow = bounds.removeFromTop(controlHeight);
    deviceInfoLabel.setBounds(infoRow.removeFromLeft(350));
    sampleRateLabel.setBounds(infoRow.removeFromLeft(120));
    channelsLabel.setBounds(infoRow.removeFromLeft(100));
    bufferLabel.setBounds(infoRow);

    bounds.removeFromTop(8);
    levelMeter.setBounds(bounds.removeFromTop(22));
}

void MainComponent::timerCallback() {
    if(captureEngine.isCapturing()){
        levelMeter.setLevel(captureEngine.getCurrentLevel());
        updateDeviceInfo();
    }
}

void MainComponent::updateDeviceInfo(){
    deviceInfoLabel.setText("Device: " + captureEngine.getDeviceName(), juce::dontSendNotification);
    sampleRateLabel.setText("Rate: " + juce::String(captureEngine.getSampleRate()) + " Hz", juce::dontSendNotification);
    channelsLabel.setText("Ch: " + juce::String(captureEngine.getChannelCount()), juce::dontSendNotification);
    bufferLabel.setText("Buf: " + juce::String(captureEngine.getBufferSize()) + " samples", juce::dontSendNotification);
}

void MainComponent::appendDiagnosticsMessage(const juce::String &message) {
    diagnosticsEditor.moveCaretToEnd();
    diagnosticsEditor.insertTextAtCaret(message + juce::newLine);
}

void MainComponent::saveSettingsFromUi() {
    settings.verboseDiagnostics = verboseDiagnosticsToggle.getToggleState();
    settings.lastLaunchTimestamp = juce::Time::getCurrentTime().toISO8601(true);

    auto errorMessage = juce::String();

    if (settingsStore.save (settings, errorMessage))
        logger.info("Settings saved: " + settingsStore.getConfigFile().getFullPathName());
    else
        logger.error("Settings save failed: " + errorMessage);

    refreshSettingsSummary();
}

void MainComponent::reloadSettingsIntoUi() {
    const auto loadResult = settingsStore.load();
    settings = loadResult.settings;
    verboseDiagnosticsToggle.setToggleState(settings.verboseDiagnostics, juce::dontSendNotification);
    refreshSettingsSummary();

    if (loadResult.recoveredFromError)
        logger.error(loadResult.message);
    else
        logger.info(loadResult.message);
}

void MainComponent::refreshSettingsSummary() {
    settingsSummaryLabel.setText ("Config: " + settingsStore.getConfigFile().getFullPathName() + " | Verbose diagnostics: " + (settings.verboseDiagnostics ? "on" : "off"), juce::dontSendNotification);
}
