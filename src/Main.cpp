#include "AppLogger.h"
#include "AppSettings.h"
#include "MainComponent.h"
#include "SettingsStore.h"

#include <juce_gui_extra/juce_gui_extra.h>

class AudioMixerApplication final : public juce::JUCEApplication {
public:
    AudioMixerApplication() = default;

    const juce::String getApplicationName() override { 
        return "AudioMixer"; 
    }

    const juce::String getApplicationVersion() override { 
        return AUDIO_MIXER_VERSION; 
    }

    bool moreThanOneInstanceAllowed() override { 
        return true; 
    }

    void initialise(const juce::String &) override {
        logger = std::make_unique<AppLogger>(settingsStore.getLogFile());
        logger->info("Startup.");

        const auto loadResult = settingsStore.load();
        settings = loadResult.settings;

        if (loadResult.recoveredFromError)
            logger->error(loadResult.message);
        else
            logger->info(loadResult.message);

        settings.lastLaunchTimestamp = juce::Time::getCurrentTime().toISO8601(true);

        if (loadResult.createdDefaults)
            saveSettings("default settings creation");

        mainWindow = std::make_unique<MainWindow>(getApplicationName(), settings, settingsStore, *logger);

        logger->setDiagnosticsSink([this](const juce::String& message){
            if (mainWindow != nullptr)
                mainWindow->appendDiagnosticsMessage(message);
        });
    }

    void shutdown() override{
        saveSettings("shutdown");

        if (logger != nullptr)
            logger->info("Shutdown.");

        mainWindow = nullptr;
        logger = nullptr;
    }

    void systemRequestedQuit() override {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

private:
    class MainWindow final : public juce::DocumentWindow{
    public:
        MainWindow(juce::String name, AppSettings &appSettings, const SettingsStore &store, AppLogger &appLogger) : 
            DocumentWindow(std::move(name),
            juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
            DocumentWindow::allButtons),
            settings(appSettings)
        {

            setUsingNativeTitleBar(true);
            setResizable(true, true);

            auto component = std::make_unique<MainComponent>(settings, store, appLogger);
            mainComponent = component.get();

            setContentOwned(component.release(), true);
            centreWithSize(settings.windowWidth, settings.windowHeight);
            setVisible(true);
        }

        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void moved() override {
            updateStoredWindowSize();
        }

        void resized() override {
            DocumentWindow::resized();
            updateStoredWindowSize();
        }

        void appendDiagnosticsMessage(const juce::String &message) {
            if (mainComponent != nullptr)
                mainComponent->appendDiagnosticsMessage(message);
        }

    private:
        void updateStoredWindowSize() {
            settings.windowWidth = getWidth();
            settings.windowHeight = getHeight();
        }

        AppSettings &settings;
        MainComponent *mainComponent{ nullptr };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    void saveSettings (const juce::String &reason) {
        if (logger == nullptr)
            return;

        auto errorMessage = juce::String();
        
        if (settingsStore.save(settings, errorMessage))
            logger->info("Settings saved during " + reason + ".");
        else
            logger->error("Settings save failed during " + reason + ": " + errorMessage);
    }

    SettingsStore settingsStore;
    AppSettings settings;
    std::unique_ptr<AppLogger> logger;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (AudioMixerApplication)
