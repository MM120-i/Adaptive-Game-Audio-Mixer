#include <juce_events/juce_events.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

#include "AudioSessionManager.h"

namespace {
    const GUID kSessionManagerIID = {
        0x77AA99A0, 0x1BD6, 0x484F,
        {0x8B, 0xC7, 0x2C, 0x65, 0x4C, 0x9A, 0x9B, 0x6F}
    };

    bool shouldSkipSession(int pid, const juce::String &name){
        if(pid == 0)
            return true;

        if(pid == GetCurrentProcessId())
            return true;

        return false;
    }

    AudioSessionInfo readSessionInfo(IAudioSessionControl *sessionControl){
        AudioSessionInfo info;

        wchar_t *displayNameWide = nullptr;
        sessionControl->GetDisplayName(&displayNameWide);
        info.displayName = juce::String(displayNameWide);
        
        if(displayNameWide) 
            CoTaskMemFree(displayNameWide);

        info.processName = info.displayName;

        unsigned long pid = 0;
        IAudioSessionControl2 *sessionControl2 = nullptr;

        if(SUCCEEDED(sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2))){
            sessionControl2->GetProcessId(&pid);
            sessionControl2->Release();
        }

            info.pid = static_cast<int>(pid);

            if(info.displayName.isEmpty() && pid > 0){
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                if(hProc){
                    wchar_t exePath[MAX_PATH];
                    DWORD len = MAX_PATH;
                    if(QueryFullProcessImageNameW(hProc, 0, exePath, &len)){
                        juce::String fullPath(exePath);
                        info.processName = fullPath.fromLastOccurrenceOf("\\", false, true);
                        info.displayName = info.processName;
                    }
                    CloseHandle(hProc);
                }
            }
        float volume = 1.0f;
        int muted = 0;
        ISimpleAudioVolume *simpleVolume = nullptr;

        if(SUCCEEDED(sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume))){
            simpleVolume->GetMasterVolume(&volume);
            simpleVolume->GetMute(&muted);
            simpleVolume->Release();
        }

        info.volume = volume;
        info.muted = (muted != 0);

        return info;
    }

    bool trySetSessionVolume(IAudioSessionControl *sessionControl, int targetPid, float volume){
        unsigned long sessionPid = 0;
        IAudioSessionControl2 *sc2 = nullptr;

        if(SUCCEEDED(sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sc2))){
            sc2->GetProcessId(&sessionPid);
            sc2->Release();
        }

        if(static_cast<int>(sessionPid) != targetPid)
            return false;

        ISimpleAudioVolume *simpleVol = nullptr;

        if(SUCCEEDED(sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVol))){
            const bool ok = SUCCEEDED(simpleVol->SetMasterVolume(volume, nullptr));
            simpleVol->Release();

            return ok;
        }

        return false;
    }
}

AudioSessionManager::AudioSessionManager(){

}

AudioSessionManager::~AudioSessionManager(){
    resetAllVolumes();
    running = false;

    if(monitorThread.joinable())
        monitorThread.join();
}

void AudioSessionManager::refreshNow(){
    forceRefresh = true;
}

void AudioSessionManager::start(){
    running = true;
    monitorThread = std::thread(&AudioSessionManager::runMonitor, this);
}

void AudioSessionManager::runMonitor(){
    const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    while(running){
        std::vector<AudioSessionInfo> currentSessions;
        IMMDeviceEnumerator *deviceEnum = nullptr;
        IMMDevice *defaultDevice = nullptr;
        IAudioSessionManager2 *sessionMgr = nullptr;
        IAudioSessionEnumerator *enumerator = nullptr;

        bool setupOk =
            SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum))
            && SUCCEEDED(deviceEnum->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice))
            && SUCCEEDED(defaultDevice->Activate(kSessionManagerIID, CLSCTX_ALL, nullptr, (void**)&sessionMgr))
            && SUCCEEDED(sessionMgr->GetSessionEnumerator(&enumerator));

        if(setupOk){
            int sessionCountSigned = 0;
            enumerator->GetCount(&sessionCountSigned);
            const size_t sessionCount = static_cast<size_t>(sessionCountSigned);

            for(size_t i = 0; i < sessionCount; i++){
                IAudioSessionControl *sessionControl = nullptr;

                if(SUCCEEDED(enumerator->GetSession(i, &sessionControl))){
                    auto info = readSessionInfo(sessionControl);

                    if(!shouldSkipSession(info.pid, info.displayName))
                        currentSessions.push_back(info);

                    sessionControl->Release();
                }
            }
        }

        if(enumerator)
            enumerator->Release();

        if(sessionMgr)
            sessionMgr->Release();

        if(defaultDevice)
            defaultDevice->Release();

        if(deviceEnum)
            deviceEnum->Release();

        {
            const juce::ScopedLock sl(sessionLock);

            if(currentSessions.size() != lastSessions.size() ||
               !std::equal(currentSessions.begin(), currentSessions.end(), lastSessions.begin(),
                    [](const auto &a, const auto &b){ 
                        return a.pid == b.pid; 
                    }
                )
            ){
                lastSessions = currentSessions;

                if(onSessionChanged)
                    juce::MessageManager::callAsync(onSessionChanged);
            }
        }

        for(size_t tick = 0; tick < 20 && running && !forceRefresh; tick++)
            Sleep(50);

        forceRefresh = false;
    }

    if(comInitialized)
        CoUninitialize();
}

std::vector<AudioSessionInfo> AudioSessionManager::getActiveSessions(){
    const juce::ScopedLock sl(sessionLock);
    return lastSessions;
}

void AudioSessionManager::setSessionVolume(int pid, float volume){
    setSessionVolumeInternal(pid, volume, true);
}

bool AudioSessionManager::setSessionVolumeInternal(int pid, float volume, bool track){
    const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    IMMDeviceEnumerator *deviceEnum = nullptr;
    IMMDevice *defaultDevice = nullptr;
    IAudioSessionManager2 *sessionMgr = nullptr;
    IAudioSessionEnumerator *enumerator = nullptr;

    bool setupOk =
        SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum))
        && SUCCEEDED(deviceEnum->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice))
        && SUCCEEDED(defaultDevice->Activate(kSessionManagerIID, CLSCTX_ALL, nullptr, (void**)&sessionMgr))
        && SUCCEEDED(sessionMgr->GetSessionEnumerator(&enumerator));

    bool changed = false;

    if(setupOk){
        int sessionCountSigned = 0;
        enumerator->GetCount(&sessionCountSigned);
        const size_t sessionCount = static_cast<size_t>(sessionCountSigned);

        for(size_t i = 0; i < sessionCount; i++){
            IAudioSessionControl *sessionControl = nullptr;

            if(SUCCEEDED(enumerator->GetSession(i, &sessionControl))){
                if(trySetSessionVolume(sessionControl, pid, volume))
                    changed = true;
                    
                sessionControl->Release();
            }
        }
    }

    if(changed && track)
        modifiedPids_.insert(pid);

    if(enumerator)
        enumerator->Release();

    if(sessionMgr)
        sessionMgr->Release();

    if(defaultDevice)
        defaultDevice->Release();

    if(deviceEnum)
        deviceEnum->Release();

    if(comInitialized)
        CoUninitialize();

    return changed;
}

void AudioSessionManager::resetAllVolumes(){
    for(auto it = modifiedPids_.begin(); it != modifiedPids_.end(); ){
        if(setSessionVolumeInternal(*it, 1.0f, false))
            it = modifiedPids_.erase(it);
        else
            ++it;
    }
}
