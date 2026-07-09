#include "MainComponent.h"

namespace {
    constexpr int padding = 20;
    constexpr int controlHeight = 30;
}

MainComponent::MainComponent (AppSettings &appSettings, const SettingsStore &store, AppLogger &appLogger): 
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

    diagnosticsEditor.setMultiLine(true);
    diagnosticsEditor.setReadOnly(true);
    diagnosticsEditor.setScrollbarsShown(true);
    diagnosticsEditor.setCaretVisible(false);
    diagnosticsEditor.setPopupMenuEnabled(true);
    diagnosticsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour{ 0xff101418 });
    diagnosticsEditor.setColour(juce::TextEditor::textColourId, juce::Colour{ 0xffd6e2ea });
    addAndMakeVisible(diagnosticsEditor);

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
