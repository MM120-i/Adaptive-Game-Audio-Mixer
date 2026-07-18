#include <juce_core/juce_core.h>
#include "ui/LevelMeter.h"

class LevelMeterTests final : public juce::UnitTest {
public:
    LevelMeterTests() : juce::UnitTest("LevelMeter", "Phase 1"){}

    void runTest() override {
        beginTest("clamp — values above 1.0 are clamped");
        {
            LevelMeter meter;
            meter.setLevel(1.5f);
            expect(meter.decay <= 1.0f);
            expect(meter.level <= 1.0f);
        }

        beginTest("clamp — negative values are clamped to 0");
        {
            LevelMeter meter;
            meter.setLevel(-0.5f);
            expect(meter.decay >= 0.0f);
            expect(meter.level >= 0.0f);
        }

        beginTest("decay — multiple zero-level calls decrease the bar");
        {
            LevelMeter meter;
            meter.setLevel(1.0f);
            expectGreaterThan(meter.decay, 0.9f);

            for (size_t i = 0; i < 10; i++)
                meter.setLevel(0.0f);

            expectLessThan(meter.decay, 1.0f);
        }

        beginTest("decay — rate matches expected decayRate constant");
        {
            LevelMeter meter;
            meter.setLevel(1.0f);          
            meter.setLevel(0.0f);          
            expectWithinAbsoluteError(meter.decay, 0.92f, 0.001f);
        }

        beginTest("peak hold — peak is stored when level spikes");
        {
            LevelMeter meter;
            meter.setLevel(0.8f);
            expectGreaterOrEqual(meter.peakDb, -60.0f);
        }

        beginTest("peak hold — very small signals produce -60 dB floor");
        {
            LevelMeter meter;
            meter.peakHoldStart = juce::Time::getMillisecondCounterHiRes() - 2000.0;
            meter.setLevel(0.0f);
            expect(meter.peakDb == -60.0f);
        }
    }
};

static LevelMeterTests levelMeterTests;
