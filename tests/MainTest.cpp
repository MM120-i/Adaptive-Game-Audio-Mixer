#include <iostream>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace { 
    class ConsoleLogger final : public juce::Logger {
        void logMessage(const juce::String &message) override {
            std::cout << message << std::endl;
        }
    };
}

int main(){
    ConsoleLogger logger;
    juce::Logger::setCurrentLogger(&logger);
    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;

    runner.runAllTests();

    for (size_t i = 0; i < runner.getNumResults(); i++)
        if (auto *result = runner.getResult(i))
            if (result->failures > 0)
                return 1;

    return 0;
}
