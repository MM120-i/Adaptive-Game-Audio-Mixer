#include <cmath>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>
#include <combaseapi.h>

#include "AudioCaptureEngine.h"

namespace {
    juce::String hrDesc(HRESULT hr){
        return "0x" + juce::String::toHexString(static_cast<int>(hr));
    }

    bool isFloatFormat(const WAVEFORMATEX *wfx){
        if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            return true;

        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfx->cbSize >= 22){
            const auto *wfxe = reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(wfx);
            return wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        }

        return false;
    }

    struct ComPtr {
        void *p = nullptr;

        ComPtr() = default;
        ComPtr(const ComPtr &) = delete;
        ComPtr &operator=(const ComPtr &) = delete;

        ~ComPtr() { 
            reset(); 
        }

        void reset() {
            if (p) {
                reinterpret_cast<IUnknown *>(p)->Release();
                p = nullptr;
            }
        }

        void **addr() { 
            return &p; 
        }

        template <typename T> T *as() const { 
            return reinterpret_cast<T *>(p); 
        }
    };

    struct TaskMemPtr{
        void *p = nullptr;

        TaskMemPtr() = default;
        TaskMemPtr(const TaskMemPtr &) = delete;
        TaskMemPtr &operator=(const TaskMemPtr &) = delete;

        ~TaskMemPtr() { 
            reset(); 
        }

        void reset() {
            if (p) {
                CoTaskMemFree(p);
                p = nullptr;
            }
        }
    };
}

AudioCaptureEngine::AudioCaptureEngine()  = default;

AudioCaptureEngine::~AudioCaptureEngine() { 
    stopCapture(); 
}

bool AudioCaptureEngine::startCapture(juce::String &errorMessage){
    const HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool weInitedCom = SUCCEEDED(coInit) && coInit != RPC_E_CHANGED_MODE;

    ComPtr mmdev;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), mmdev.addr());

    if (FAILED(hr)){
        errorMessage = "Failed to create MMDeviceEnumerator.";

        if(weInitedCom) 
            CoUninitialize();

        return false;
    }

    ComPtr endpoint;
    hr = reinterpret_cast<IMMDeviceEnumerator *>(mmdev.p)->GetDefaultAudioEndpoint(eRender, eConsole, reinterpret_cast<IMMDevice **>(endpoint.addr()));
    mmdev.reset(); 

    if(FAILED(hr)){
        errorMessage = "No active playback device. Connect speakers/headphones.";

        if(weInitedCom) 
            CoUninitialize();

        return false;
    }

    ComPtr props;
    auto *dev = reinterpret_cast<IMMDevice *>(endpoint.p);

    if(SUCCEEDED(dev->OpenPropertyStore(STGM_READ, reinterpret_cast<IPropertyStore **>(props.addr())))){
        PROPVARIANT nameVar;
        PropVariantInit(&nameVar);

        if(SUCCEEDED(reinterpret_cast<IPropertyStore *>(props.p)->GetValue(PKEY_Device_FriendlyName, &nameVar)))
            deviceName = nameVar.pwszVal;

        PropVariantClear(&nameVar);
    }

    props.reset(); 
    ComPtr client;
    hr = dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(client.addr()));

    if (FAILED(hr)){
        errorMessage = "Failed to activate IAudioClient.";

        if(weInitedCom) 
            CoUninitialize();

        return false;
    }

    auto *audio = reinterpret_cast<IAudioClient *>(client.p);
    TaskMemPtr wfxOwn;
    WAVEFORMATEX *wfx = nullptr;
    hr = audio->GetMixFormat(&wfx);
    wfxOwn.p = wfx;

    if(FAILED(hr)){
        errorMessage = "Failed to get mix format.";

        if(weInitedCom)
            CoUninitialize();

        return false;
    }

    sampleRate = static_cast<int>(wfx->nSamplesPerSec);
    channelCount = static_cast<int>(wfx->nChannels);

    diagLog = "Device: " + deviceName
            + "\n  Sample rate: " + juce::String(sampleRate.load()) + " Hz"
            + "\n  Channels: " + juce::String(channelCount.load())
            + "\n  Bit depth: " + juce::String(wfx->wBitsPerSample)
            + "\n  Format: " + (isFloatFormat(wfx) ? "IEEE float" : "PCM");

    if (!isFloatFormat(wfx)){
        errorMessage = "Unsupported audio format. WASAPI loopback must be IEEE float.";

        if(weInitedCom)
            CoUninitialize();

        return false;
    }

    hr = audio->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, wfx, nullptr);
    wfxOwn.reset(); 

    if (FAILED (hr)){
        errorMessage = "WASAPI loopback not available. (" + hrDesc(hr) + ")";
        
        if(weInitedCom) 
            CoUninitialize();

        return false;
    }

    ComPtr capture;
    hr = audio->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void **>(capture.addr()));
    
    if (FAILED(hr)){
        errorMessage = "Failed to get IAudioCaptureClient.";
        
        if(weInitedCom)
            CoUninitialize();

        return false;
    }

    UINT32 bufFrames = 0;
    hr = audio->GetBufferSize(&bufFrames);

    if (FAILED(hr)){
        errorMessage = "GetBufferSize failed. (" + hrDesc(hr) + ")";
        
        if(weInitedCom)
            CoUninitialize();

        return false;
    }

    bufferSize = static_cast<int>(bufFrames);
    hr = audio->Start();

    if (FAILED(hr)){
        errorMessage = "Start failed. (" + hrDesc(hr) + ")";
        
        if(weInitedCom)
            CoUninitialize();

        return false;
    }

    endpoint.reset();

    auto *capRaw = reinterpret_cast<IAudioCaptureClient *>(capture.p);
    auto *cliRaw = audio;
    capture.p = nullptr;
    client.p  = nullptr;

    auto loop = [this] (IAudioCaptureClient *cap, IAudioClient *cli) {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        while(capturing.load(std::memory_order_relaxed)){
            Sleep(10); 

            while(true){
                BYTE *data = nullptr;
                UINT32 frm = 0;
                DWORD flags = 0;
                HRESULT r = cap->GetBuffer(&data, &frm, &flags, nullptr, nullptr);

                if (r == AUDCLNT_S_BUFFER_EMPTY)
                    break;

                if (FAILED(r)){
                    captureError = "Capture device error: " + hrDesc(r);
                    capturing.store(false, std::memory_order_relaxed);
                    break;
                }

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT){
                    currentLevel.store(0.0f, std::memory_order_relaxed);
                }
                else if(frm > 0 && data != nullptr){
                    const auto *samples = reinterpret_cast<const float *>(data);
                    const int chs = channelCount.load();
                    float sum = 0.0f;

                    for(UINT32 i = 0; i < frm; ++i){
                        float mono = 0.0f;

                        for(size_t c = 0; c < chs; ++c)
                            mono += samples[i * chs + c];

                        mono /= static_cast<float>(chs);
                        sum += mono * mono;
                    }

                    const float rms = std::sqrt(sum / static_cast<float>(frm));
                    constexpr float alpha = 0.3f;
                    smoothedLevel = (alpha * rms) + ((1.0f - alpha) * smoothedLevel);
                    currentLevel.store(smoothedLevel, std::memory_order_relaxed);
                }

                cap->ReleaseBuffer(frm);
            }
        }

        cli->Stop();
        pendingCapture = cap;
        pendingClient = cli;
        CoUninitialize();
    };

    capturing = true;
    captureThread = std::thread(loop, capRaw, cliRaw);

    return true;
}

void AudioCaptureEngine::stopCapture(){
    capturing = false;

    if (captureThread.joinable())
        captureThread.join();

    if (pendingCapture){
        reinterpret_cast<IAudioCaptureClient *>(pendingCapture)->Release();
        pendingCapture = nullptr;
    }

    if (pendingClient){
        reinterpret_cast<IAudioClient *>(pendingClient)->Release();
        pendingClient = nullptr;
    }

    currentLevel = 0.0f;
}

bool AudioCaptureEngine::isCapturing() const { 
    return capturing.load(); 
}
