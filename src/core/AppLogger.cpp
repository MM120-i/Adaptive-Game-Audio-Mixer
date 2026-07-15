#include "AppLogger.h"
#include <juce_events/juce_events.h>

namespace {
    juce::String currentTimestamp(){
        return juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S");
    }
}

AppLogger::AppLogger(juce::File file) : logFile(std::move(file)) {
    logFile.getParentDirectory().createDirectory();
}

void AppLogger::setDiagnosticsSink(DiagnosticsSink sink){
    const juce::ScopedLock sl(lock);
    diagnosticsSink = std::move(sink);

    if (diagnosticsSink == nullptr)
        return;

    for (const auto &message : bufferedMessages)
        diagnosticsSink(message);
}

void AppLogger::info(const juce::String &message){
    write("INFO", message);
}

void AppLogger::error(const juce::String& message){
    write("ERROR", message);
}

void AppLogger::write (const juce::String &level, const juce::String &message) {
    const juce::ScopedLock sl(lock);
    const auto line = "[" + currentTimestamp() + "] [" + level + "] " + message;
    
    logFile.appendText(line + juce::newLine);
    bufferedMessages.add(line);

    while (bufferedMessages.size() > 200)
        bufferedMessages.remove(0);

    if (diagnosticsSink != nullptr){
        const auto sink = diagnosticsSink;

        juce::MessageManager::callAsync([sink, line] { 
            sink (line); 
        });
    }
}
