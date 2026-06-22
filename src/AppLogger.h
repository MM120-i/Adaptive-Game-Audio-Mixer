#pragma once

#include <functional>
#include <juce_core/juce_core.h>

class AppLogger{
public:
    using DiagnosticsSink = std::function<void (const juce::String&)>;

    explicit AppLogger (juce::File logFile);

    void setDiagnosticsSink (DiagnosticsSink sink);
    void info (const juce::String& message);
    void error (const juce::String& message);

private:
    void write (const juce::String& level, const juce::String& message);

    juce::File logFile;
    DiagnosticsSink diagnosticsSink;
    juce::StringArray bufferedMessages;
};
