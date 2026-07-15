#include "AudioCaptureEngine.h"

AudioCaptureEngine::AudioCaptureEngine(){
    deviceManager.initialiseWithDefaultDevices(0, 2);
}

AudioCaptureEngine::~AudioCaptureEngine(){
    stopCapture();
}

bool AudioCaptureEngine::startCapture(juce::String &errorMessage){
    auto *device = deviceManager.getCurrentAudioDevice();

    if(!device){
        errorMessage = "No audio device found";
        return false;
    }

    auto setUp = deviceManager.getAudioDeviceSetup();
    setUp.inputDeviceName = setUp.outputDeviceName;
    setUp.useDefaultInputChannels = true;

    const auto result = deviceManager.setAudioDeviceSetup(setUp, true);

    if(result.isNotEmpty()){
        errorMessage = result;
        return false;
    }

    deviceManager.addAudioCallback(this);
    deviceName = setUp.outputDeviceName;
    capturing = true;

    return true;
}

void AudioCaptureEngine::stopCapture(){
    deviceManager.removeAudioCallback(this);
    capturing = false;
    currentLevel = 0.0f;
}

bool AudioCaptureEngine::isCapturing() const {
    return capturing.load();
}

juce::String AudioCaptureEngine::getDeviceName() const {
    return deviceName;
}

int AudioCaptureEngine::getSampleRate() const {
    return sampleRate.load();
}

int AudioCaptureEngine::getChannelCount() const {
    return channelCount.load();
}

int AudioCaptureEngine::getBufferSize() const {
    return bufferSize.load();
}

float AudioCaptureEngine::getCurrentLevel() const {
    return currentLevel.load();
}

/**
 * \note: THE FOLLOWING FUNCTIONS GOTTA RUN ON AUDIO THREAD, SO IT GOTTA BE FAST AF NO BLOCKING
 */
void AudioCaptureEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* /*outputChannelData*/, int /*numOutputChannels*/, int numSamples, const juce::AudioIODeviceCallbackContext& /*context*/){
    if(numInputChannels == 0)
        return;

    float sum = 0.0f;

    for(size_t sample = 0; sample < numSamples; ++sample){
        float mono = 0.0f;

        for(size_t ch = 0; ch < numInputChannels; ++ch)
            mono += inputChannelData[ch][sample];

        mono /= static_cast<float>(numInputChannels);
        sum += mono * mono;
    }

    const float rms = std::sqrt(sum / static_cast<float>(numSamples));
    constexpr float alpha = 0.3f;
    smoothLevel = (alpha * rms) + ((1.0f - alpha) * smoothLevel);
    currentLevel.store(smoothLevel, std::memory_order_relaxed);
}

void AudioCaptureEngine::audioDeviceAboutToStart(juce::AudioIODevice *device){
    sampleRate = device->getCurrentSampleRate();
    channelCount = device->getActiveInputChannels().countNumberOfSetBits();
    bufferSize = device->getCurrentBufferSizeSamples();
}

void AudioCaptureEngine::audioDeviceStopped(){
    currentLevel = 0.0f;
    smoothLevel = 0.0f;
}