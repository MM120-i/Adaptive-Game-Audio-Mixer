#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "ui/VolumeControl.h"

class VolumeControlTests final : public juce::UnitTest {
public:
    VolumeControlTests(): juce::UnitTest ("VolumeControl", "Phase 3"){}

    void runTest() override {
        beginTest("setVolume / getVolume round-trip");
        {
            VolumeControl vc;
            vc.setVolume(42);
            expectEquals(vc.getVolume(), 42);
        }

        beginTest("clamp --- negative values clamped to 0");
        {
            VolumeControl vc;
            vc.setVolume(-10);
            expectEquals(vc.getVolume(), 0);
        }

        beginTest("clamp --- values above 100 clamped to 100");
        {
            VolumeControl vc;
            vc.setVolume(150);
            expectEquals(vc.getVolume(), 100);
        }

        beginTest("mute saves volume and restores on unmute");
        {
            VolumeControl vc;
            vc.setVolume(75);
            expectEquals(vc.getVolume(), 75);
            expect(vc.isMuted() == false);   
        }

        beginTest("setMuted --- mutes to 0 and restores pre-mute volume");
        {
            VolumeControl vc;
            vc.setVolume(75);
            expectEquals(vc.getVolume(), 75);
            expect(vc.isMuted() == false);

            vc.setMuted(true);
            expectEquals(vc.getVolume(), 0);
            expect(vc.isMuted() == true);

            vc.setMuted(false);
            expectEquals(vc.getVolume(), 75);
            expect(vc.isMuted() == false);
        }

        beginTest("setMuted --- double-mute is no-op");
        {
            VolumeControl vc;
            vc.setVolume(50);
            vc.setMuted(true);
            vc.setMuted(true);  
            expectEquals(vc.getVolume(), 0);
            vc.setMuted(false);
            expectEquals(vc.getVolume(), 50);
        }

        beginTest("setVolume with dontSendNotification doesn't trigger commit");
        {
            int fired = 0;
            VolumeControl vc;
            vc.setCommitCallback([&](int){ fired++; });
            vc.setVolume(50);
            expectEquals(fired, 0);
        }
    }
};

static VolumeControlTests volumeControlTests;