#include <juce_core/juce_core.h>

#include "ui/OverlayHud.h"
#include "core/SpotifyClient.h"
#include "ui/AudioBalancer.h"
#include "audio/AudioSessionManager.h"

class OverlayHudTests final : public juce::UnitTest {
public:
    OverlayHudTests(): juce::UnitTest("OverlayHudTests", "OverlayHud"){}

#pragma warning(push)
#pragma warning(disable: 6262)
    void runTest() override {
        beginTest("constructor does not crash");
        {
            AudioSessionManager mgr;
            SpotifyClient sc;
            AudioBalancer balancer(mgr);
            OverlayHud hud(sc, balancer);
            expect(true);
        }

        beginTest("toggle does not crash");
        {
            AudioSessionManager mgr;
            SpotifyClient sc;
            AudioBalancer balancer(mgr);
            OverlayHud hud(sc, balancer);
            hud.toggle();
            hud.toggle();
            hud.toggle();
            expect(true);
        }

        beginTest("flash does not crash");
        {
            AudioSessionManager mgr;
            SpotifyClient sc;
            AudioBalancer balancer(mgr);
            OverlayHud hud(sc, balancer);
            hud.flash();
            hud.flash();
            expect(true);
        }

        beginTest("flash then toggle does not crash");
        {
            AudioSessionManager mgr;
            SpotifyClient sc;
            AudioBalancer balancer(mgr);
            OverlayHud hud(sc, balancer);
            hud.flash();
            hud.toggle();
            hud.toggle();
            expect(true);
        }
    }
#pragma warning(pop)
};

static OverlayHudTests overlayHudTests;
