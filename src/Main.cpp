#include "core/AppLogger.h"
#include "core/AppSettings.h"
#include "ui/MainComponent.h"
#include "ui/MixerLookAndFeel.h"
#include "core/SettingsStore.h"
#include "core/GlobalHotkeys.h"
#include "core/SystemTray.h"
#include "ui/VolumeNotification.h"
#include "ui/OverlayHud.h"

#include <juce_gui_extra/juce_gui_extra.h>

class AudioMixerApplication final : public juce::JUCEApplication {
public:
    std::unique_ptr<SystemTray> trayIcon;
    std::unique_ptr<OverlayHud> overlayHud;

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

    bool canUseHotkeys(){
        if(!mainWindow)
            return false;

        const auto &mc = mainWindow->getMainComponent();

        if(!mc.captureEngine.isCapturing()){
            VolumeNotification::show("Start capture first");
            return false;
        }

        if(!mc.spotifyClient.isAuthenticated()){
            VolumeNotification::show("Connect Spotify first");
            return false;
        }

        return true;
    }

    void adjustVolume(float delta) {
        if(!canUseHotkeys())
            return;

        auto &balancer = mainWindow->getMainComponent().audioBalancer;
        float current = static_cast<float>(balancer.getCrossFaderValue());
        float newVal = std::clamp(current + delta, 0.0f, 1.0f);
        balancer.setCrossFaderValue(newVal);

        int spotPct = static_cast<int>((1.0f - newVal) * 100);
        int gamePct = static_cast<int>(newVal * 100);
        VolumeNotification::show("Spotify: " + juce::String(spotPct) + "%  Game: " + juce::String(gamePct) + "%");

        flashHud();
    }

    void toggleMute() {
        if(!canUseHotkeys())
            return;

        auto &vc = mainWindow->getMainComponent().volumeControl;
        vc.setMuted(!vc.isMuted());

        VolumeNotification::show(vc.isMuted() ? juce::String("Muted") : juce::String("Volume: ") + juce::String(vc.getVolume()) + "%");

        if(trayIcon)
            trayIcon->updateMenuText(vc.isMuted());

        flashHud();
    }

    void togglePlayPause() {
        if(!canUseHotkeys())
            return;

        auto &sc = mainWindow->getMainComponent().spotifyClient;
        sc.setPlaying(!sc.isPlaying());
        VolumeNotification::show(sc.isPlaying() ? juce::String("Playing") : juce::String("Paused"));
        
        flashHud();
    }

    void skipNext() {
        if(!canUseHotkeys())
            return;

        mainWindow->getMainComponent().spotifyClient.skipNext();
        flashHud();
    }

    void toggleWindow() {
        if(!mainWindow) 
            return;

        mainWindow->setVisible(!mainWindow->isVisible());
    }

    void toggleHud() {
        if(!mainWindow)
            return;

        if(!overlayHud){
            auto &mc = mainWindow->getMainComponent();
            overlayHud = std::make_unique<OverlayHud>(mc.spotifyClient, mc.audioBalancer);
        }

        overlayHud->toggle();
    }

    void flashHud() {
        if(!mainWindow) 
            return;

        if(!overlayHud){
            auto &mc = mainWindow->getMainComponent();
            overlayHud = std::make_unique<OverlayHud>(mc.spotifyClient, mc.audioBalancer);
        }

        overlayHud->flash();
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

        hotkeys = std::make_unique<GlobalHotkeyManager>();

        hotkeys->add(MOD_CONTROL, VK_UP, [this]{ 
            adjustVolume(-0.05f); 
        });

        hotkeys->add(MOD_CONTROL, VK_DOWN, [this]{ 
            adjustVolume(+0.05f); 
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

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, VK_LEFT, [this]{ 
            if(!canUseHotkeys())
                return;
            mainWindow->getMainComponent().spotifyClient.skipPrevious();
            flashHud(); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, 'O', [this]{
            toggleWindow(); 
        });

        hotkeys->add(MOD_CONTROL | MOD_SHIFT, 'H', [this]{ 
            toggleHud(); 
        });

        trayIcon = std::make_unique<SystemTray>();

        trayIcon->onShow = [this] {
            mainWindow->setVisible(!mainWindow->isVisible());

            if(mainWindow->isVisible())
                mainWindow->toFront(true);
        };

        trayIcon->onQuit = [this] {
            quit();
        };

        trayIcon->onMute = [this] {
            toggleMute();
        };

        trayIcon->create();

        if(settings.runAtStartup)
            setRunStartup(true);
    }

    void shutdown() override {
        overlayHud.reset();
        
        if(trayIcon)
            trayIcon->destroy();

        saveSettings("shutdown");

        if (logger)
            logger->info("Shutdown.");

        mainWindow = nullptr;
        lookAndFeel = nullptr;
        logger = nullptr;
    }

    void setRunStartup(bool enable){
        juce::String exePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();
        juce::String keyPath = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

        if(enable)
            juce::WindowsRegistry::setValue(keyPath + "\\AudioMixer", exePath);
        else
            juce::WindowsRegistry::deleteValue(keyPath + "\\AudioMixer");
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
            setResizeLimits(700, 700, 4000, 5000);

            auto component = std::make_unique<MainComponent>(settings, store, appLogger);
            mainComponent = component.get();

            setContentOwned(component.release(), true);
            settings.windowWidth = juce::jlimit(700, 4000, settings.windowWidth);
            settings.windowHeight = juce::jlimit(700, 5000, settings.windowHeight);
            centreWithSize(settings.windowWidth, settings.windowHeight);
            setVisible(true);
        }

        void closeButtonPressed() override {
            setVisible(false);
        }

        void moved() override {
            updateStoredWindowSize();
        }

        void resized() override {
            DocumentWindow::resized();
            updateStoredWindowSize();
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

START_JUCE_APPLICATION(AudioMixerApplication)
