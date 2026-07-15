#pragma once

#include <functional>
#include <juce_core/juce_core.h>

class AppLogger{
public:
    using DiagnosticsSink = std::function<void(const juce::String &)>;

    explicit AppLogger(juce::File); 

    void setDiagnosticsSink(DiagnosticsSink);
    void info(const juce::String &);
    void error(const juce::String &);

private:
    void write(const juce::String &, const juce::String &);

    juce::File logFile;
    DiagnosticsSink diagnosticsSink;
    juce::StringArray bufferedMessages;
};
