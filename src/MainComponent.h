#pragma once

#include "AppLogger.h"
#include "AppSettings.h"
#include "SettingsStore.h"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component {
public:
    MainComponent (AppSettings &, const SettingsStore &, AppLogger &);
    ~MainComponent() override;

    void paint (juce::Graphics &) override;
    void resized() override;
    void appendDiagnosticsMessage(const juce::String &);

private:
    void saveSettingsFromUi();
    void reloadSettingsIntoUi();
    void refreshSettingsSummary();

    AppSettings &settings;
    const SettingsStore &settingsStore;
    AppLogger &logger;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::Label settingsSummaryLabel;
    juce::ToggleButton verboseDiagnosticsToggle;
    juce::TextButton saveSettingsButton{ "Save Settings" };
    juce::TextButton reloadSettingsButton{ "Reload Settings" };
    juce::TextEditor diagnosticsEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
