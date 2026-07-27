#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>
#include <set>
#include <thread>
#include <vector>

struct AudioSessionInfo {
    juce::String processName;
    juce::String displayName;
    int pid;
    float volume;
    bool muted;
};

class AudioSessionManager {
private:
    void runMonitor();
    void setSessionVolumeInternal(int, float, bool);

    std::thread monitorThread;
    std::atomic<bool> running{false};
    std::atomic<bool> forceRefresh{false};
    std::vector<AudioSessionInfo> lastSessions;
    std::set<int> modifiedPids_;
    juce::CriticalSection sessionLock;

public:
    AudioSessionManager();
    ~AudioSessionManager();

    void start();
    std::vector<AudioSessionInfo> getActiveSessions();
    void setSessionVolume(int, float);
    void resetAllVolumes();
    void refreshNow();

    std::function<void()> onSessionChanged;
};