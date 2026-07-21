#include "core/AppLogger.h"
#include "core/AppSettings.h"
#include "ui/MainComponent.h"
#include "ui/MixerLookAndFeel.h"
#include "core/SettingsStore.h"
#include "core/GlobalHotkeys.h"

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

    void adjustVolume(int delta) {
        if(!mainWindow) return;
        int vol = mainWindow->getMainComponent().volumeControl.getVolume();
        vol = std::clamp(vol + delta, 0, 100);
        mainWindow->getMainComponent().volumeControl.setVolume(vol);
        mainWindow->getMainComponent().spotifyClient.setVolume(vol);
    }

    void toggleMute() {
        if(!mainWindow) return;
        auto &vc = mainWindow->getMainComponent().volumeControl;
        if(vc.isMuted())
            vc.setVolume(vc.getVolume()); // unmute restores via internal state
        else
            vc.setVolume(0); // mute
    }

    void togglePlayPause() {
        if(!mainWindow) return;
        auto &sc = mainWindow->getMainComponent().spotifyClient;
        sc.setPlaying(!sc.isPlaying());
    }

    void skipNext() {
        if(!mainWindow) return;
        mainWindow->getMainComponent().spotifyClient.skipNext();
    }

    void applyPreset(int idx) {
        if(!mainWindow) return;
        if(idx < 0 || idx >= settings.volumePresets.size()) return;
        int vol = settings.volumePresets[idx].volume;
        mainWindow->getMainComponent().volumeControl.animateToVolume(vol, 300);
    }

    void toggleWindow() {
        if(!mainWindow) return;
        mainWindow->setVisible(!mainWindow->isVisible());
    }

    void toggleHud() {
        // Phase 8 — HUD overlay, no-op for now
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
            
        lookAndFeel = std::make_unique<MixerLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());
        mainWindow = std::make_unique<MainWindow>(getApplicationName(), settings, settingsStore, *logger);

        logger->setDiagnosticsSink([this](const juce::String &message){
            if (mainWindow != nullptr)
                mainWindow->appendDiagnosticsMessage(message);
        });

        hotkeys = std::make_unique<GlobalHotkeyManager>();

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, VK_UP, [this] { 
            adjustVolume(+5); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, VK_DOWN, [this]{ 
            adjustVolume(-5); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, 'M', [this]{ 
            toggleMute(); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, 'P', [this]{ 
            togglePlayPause(); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, VK_RIGHT, [this]{ 
            skipNext(); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, '1', [this]{ 
            applyPreset(0); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, '2', [this]{ 
            applyPreset(1); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, '3', [this]{
            applyPreset(2); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, '4', [this]{ 
            applyPreset(3); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, 'O', [this]{
            toggleWindow(); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, 'H', [this]{ 
            toggleHud(); 
        });
    }

    void shutdown() override{
        saveSettings("shutdown");

        if (logger != nullptr)
            logger->info("Shutdown.");

        mainWindow = nullptr;
        lookAndFeel = nullptr;
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

        MainComponent& getMainComponent() { return *mainComponent; }

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
    std::unique_ptr<MixerLookAndFeel> lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<GlobalHotkeyManager> hotkeys;
};

START_JUCE_APPLICATION (AudioMixerApplication)
