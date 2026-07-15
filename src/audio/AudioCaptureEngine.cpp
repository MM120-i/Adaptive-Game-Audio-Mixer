#include <cmath>
#include "AudioCaptureEngine.h"

AudioCaptureEngine::AudioCaptureEngine(){}

AudioCaptureEngine::~AudioCaptureEngine() {
    stopCapture();
}

static juce::AudioIODeviceType *findWASAPI(juce::AudioDeviceManager &dm) {
    for(auto *type : dm.getAvailableDeviceTypes()){
        const auto n = type->getTypeName();

        if (n.containsIgnoreCase("Windows Audio") || n.containsIgnoreCase("WASAPI"))
            return type;
    }

    return nullptr;
}

bool AudioCaptureEngine::startCapture(juce::String &errorMessage){
    auto *wasapiType = findWASAPI(deviceManager);

    if (wasapiType == nullptr){
        errorMessage = "WASAPI audio backend not available.";
        return false;
    }

    deviceManager.setCurrentAudioDeviceType(wasapiType->getTypeName(), true);
    wasapiType->scanForDevices();

    const auto inputDevices = wasapiType->getDeviceNames(true);  
    const auto outputDevices = wasapiType->getDeviceNames(false);  

    deviceManagerLog  = "WASAPI outputs (" + juce::String (outputDevices.size()) + "): ";

    for(size_t i = 0; i < outputDevices.size(); ++i)
        deviceManagerLog += "\n  [" + juce::String(i) + "] " + outputDevices[i];

    deviceManagerLog += "\nWASAPI inputs (" + juce::String (inputDevices.size()) + "): ";

    for (size_t i = 0; i < inputDevices.size(); ++i)
        deviceManagerLog += "\n  [" + juce::String (i) + "] " + inputDevices[i];

    juce::String loopbackName;
    int priority = 4;

    for(size_t ni = 0; ni < inputDevices.size(); ++ni){
        const auto& in = inputDevices[ni];

        for(size_t no = 0; no < outputDevices.size(); ++no){
            if (in == outputDevices[no] && priority > 1){
                loopbackName = in;
                priority = 1;
                break;
            }
        }

        if (priority > 2){
            const auto lower = in.toLowerCase();

            if (lower.contains("loopback") || 
                lower.contains("stereo mix") || 
                lower.contains("what u hear") || 
                lower.contains("wave out"))
            {
                loopbackName = in;
                priority = 2;
            }
        }

        if (priority > 3){
            loopbackName = in;
            priority = 3;
        }
    }

    if (loopbackName.isEmpty()){
        errorMessage = "No suitable input device found." + deviceManagerLog;
        return false;
    }

    deviceName = loopbackName;
    deviceManagerLog += "\n\nSelected: " + deviceName + " (priority=" + juce::String(priority) + ")";

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.inputDeviceName = deviceName;
    setup.outputDeviceName.clear();
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = false;
    setup.sampleRate = 0;
    setup.bufferSize = 0;

    const auto result = deviceManager.setAudioDeviceSetup(setup, true);

    if (result.isNotEmpty()){
        deviceManager.closeAudioDevice();
        errorMessage = result + deviceManagerLog;
        return false;
    }

    deviceManager.addAudioCallback(this);
    capturing = true;
    return true;
}

void AudioCaptureEngine::stopCapture(){
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
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

//  AUDIO THREAD — fast, no blocking, no allocations
void AudioCaptureEngine::audioDeviceIOCallbackWithContext(const float * const *inputChannelData,
                                                          int numInputChannels,
                                                          float* const* /*outputChannelData*/,
                                                          int /*numOutputChannels*/,
                                                          int numSamples,
                                                          const juce::AudioIODeviceCallbackContext & /*context*/)
{
    if (numInputChannels == 0)
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
    smoothedLevel = (alpha * rms) + ((1.0f - alpha) * smoothedLevel);
    currentLevel.store(smoothedLevel, std::memory_order_relaxed);
}

void AudioCaptureEngine::audioDeviceAboutToStart(juce::AudioIODevice *device){
    sampleRate = device->getCurrentSampleRate();
    channelCount = device->getActiveInputChannels().countNumberOfSetBits();
    bufferSize = device->getCurrentBufferSizeSamples();
}

void AudioCaptureEngine::audioDeviceStopped(){
    currentLevel = 0.0f;
    smoothedLevel = 0.0f;
}
