#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>
#include <thread>

class AudioCaptureEngine {
private:
    std::atomic<float> currentLevel{ 0.0f };
    std::atomic<int> sampleRate{ 0 };
    std::atomic<int> channelCount{ 0 };
    std::atomic<int> bufferSize{ 0 };
    std::atomic<bool> capturing{ false };

    juce::String deviceName;
    juce::String diagLog;

    float smoothedLevel{ 0.0f };

    std::thread captureThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCaptureEngine)

public:
    AudioCaptureEngine();
    ~AudioCaptureEngine();

    bool startCapture (juce::String &);
    void stopCapture();
    bool isCapturing() const;

    juce::String getDeviceName() const { 
        return deviceName; 
    }

    juce::String getDeviceDiagnostics() const { 
        return diagLog; 
    }

    int getSampleRate() const { 
        return sampleRate.load(); 
    }

    int getChannelCount() const { 
        return channelCount.load(); 
    }

    int getBufferSize() const { 
        return bufferSize.load(); 
    }

    float getCurrentLevel() const { 
        return currentLevel.load(); 
    }
};
