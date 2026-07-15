#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

class AudioCaptureEngine : private juce::AudioIODeviceCallback {
private:
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice *) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager deviceManager;

    std::atomic<float> currentLevel {0.0f};
    std::atomic<int> sampleRate {0};
    std::atomic<int> channelCount {0};
    std::atomic<int> bufferSize {0};
    std::atomic<bool> capturing {false};

    juce::String deviceName;
    float smoothLevel {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCaptureEngine)


public:
    AudioCaptureEngine();
    ~AudioCaptureEngine();

    bool startCapture(juce::String &);
    void stopCapture();
    bool isCapturing() const;

    juce::String getDeviceName() const;
    int getSampleRate() const;
    int getChannelCount() const;
    int getBufferSize() const;
    float getCurrentLevel() const;
};