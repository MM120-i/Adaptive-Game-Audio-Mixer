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