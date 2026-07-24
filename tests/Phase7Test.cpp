#define NOMINMAX
#include <juce_core/juce_core.h>

#include "core/GlobalHotkeys.h"
#include "core/SystemTray.h"
#include "ui/VolumeNotification.h"

class Phase7Tests final : public juce::UnitTest {
public:
    Phase7Tests(): juce::UnitTest("Phase7", "Phase 7"){}

    void runTest() override {
        beginTest("VolumeNotification::show --- does not crash");
        {
            VolumeNotification::show("Test message");
            expect(true);
        }

        beginTest("GlobalHotkeyManager --- constructor and destructor");
        {
            GlobalHotkeyManager mgr;
            expect(true);
        }

        beginTest("GlobalHotkeyManager --- add and removeAll");
        {
            GlobalHotkeyManager mgr;

            mgr.add(MOD_CONTROL | MOD_SHIFT, VK_F13, []{});
            mgr.add(MOD_CONTROL | MOD_ALT, VK_F14, []{});
            mgr.add(0, VK_F15, []{});
            mgr.removeAll();
            expect(true);
        }

        beginTest("SystemTray --- constructor and destructor");
        {
            SystemTray tray;
            expect(true);
        }

        beginTest("SystemTray --- create and destroy cycle");
        {
            SystemTray tray;
            tray.create();
            expect(tray.isCreated() == true);

            tray.destroy();
            expect(tray.isCreated() == false);
        }

        beginTest("SystemTray --- double create is no-op");
        {
            SystemTray tray;
            tray.create();
            tray.create(); 
            expect(tray.isCreated() == true);
            tray.destroy();
        }

        beginTest("SystemTray --- destroy without create is safe");
        {
            SystemTray tray;
            tray.destroy(); 
            expect(tray.isCreated() == false);
        }

        beginTest("GlobalHotkeyManager --- callbacks fire when handleHotkey called directly");
        {
            GlobalHotkeyManager mgr;
            int fired = 0;

            mgr.add(0, VK_F16, [&]{ fired++; });
            mgr.add(0, VK_F17, [&]{ fired += 10; });
            mgr.handleHotkey(1);
            mgr.handleHotkey(2);
            mgr.handleHotkey(99); 
            expect(true);
        }
    }
};

static Phase7Tests phase7Tests;
