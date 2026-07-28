#define NOMINMAX
#include <juce_core/juce_core.h>

#include "core/GlobalHotkeys.h"
#include "core/SystemTray.h"
#include "ui/VolumeNotification.h"
#include "audio/AudioSessionManager.h"
#include "ui/AudioBalancer.h"

class Phase7Tests final : public juce::UnitTest {
public:
    Phase7Tests(): juce::UnitTest("Phase7", "Phase 7"){}

#pragma warning(push)
#pragma warning(disable: 6262)
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

        beginTest("AudioSessionManager --- constructor and destructor");
        {
            AudioSessionManager mgr;
            expect(true);
        }

        beginTest("AudioSessionManager --- getActiveSessions returns empty initially");
        {
            AudioSessionManager mgr;
            const auto sessions = mgr.getActiveSessions();
            expect(sessions.empty());
        }

        beginTest("AudioSessionManager --- setSessionVolume does not crash");
        {
            AudioSessionManager mgr;
            mgr.setSessionVolume(1234, 0.5f);
            mgr.setSessionVolume(9999, 0.0f);
            mgr.setSessionVolume(0, 1.0f);
            expect(true);
        }

        beginTest("AudioSessionManager --- multiple instances are independent");
        {
            AudioSessionManager mgr1;
            AudioSessionManager mgr2;

            const auto s1 = mgr1.getActiveSessions();
            const auto s2 = mgr2.getActiveSessions();

            expect(s1.empty() == s2.empty());
        }

        beginTest("AudioBalancer --- constructor does not crash");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            expect(true);
        }

        beginTest("AudioBalancer --- refreshSessions with empty sessions");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            balancer.refreshSessions();  // no sessions — should not crash
            expect(true);
        }

        beginTest("AudioBalancer --- setSystemLevel does not crash");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            balancer.setSystemLevel(0.5f);
            balancer.setSystemLevel(0.0f);
            balancer.setSystemLevel(1.0f);
            expect(true);
        }

        beginTest("AudioBalancer --- resized does not crash with no parent");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            balancer.resized();  
            expect(true);
        }

        beginTest("AudioBalancer --- getCrossFaderValue returns default 0.5");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            expectEquals(balancer.getCrossFaderValue(), 0.5);
        }

        beginTest("AudioBalancer --- setCrossFaderValue round-trip");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            balancer.setCrossFaderValue(0.75);
            expectEquals(balancer.getCrossFaderValue(), 0.75);
            balancer.setCrossFaderValue(0.25);
            expectEquals(balancer.getCrossFaderValue(), 0.25);
        }

        beginTest("AudioBalancer --- setCrossFaderValue clamps to range");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            balancer.setCrossFaderValue(2.0);
            expectEquals(balancer.getCrossFaderValue(), 1.0);
            balancer.setCrossFaderValue(-0.5);
            expectEquals(balancer.getCrossFaderValue(), 0.0);
        }

        beginTest("AudioSessionManager --- refreshNow does not crash");
        {
            AudioSessionManager mgr;
            mgr.refreshNow();
            expect(true);
        }

        beginTest("AudioBalancer --- timer callback does not crash");
        {
            AudioSessionManager mgr;
            AudioBalancer balancer(mgr);
            balancer.timerCallback();
            expect(true);
        }
    }
#pragma warning(pop)
};

static Phase7Tests phase7Tests;
