#include "GlobalHotkeys.h"

#include <windows.h>
#include <juce_events/juce_events.h>

namespace {
    LRESULT CALLBACK hotkeyWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
        if(msg == WM_HOTKEY){
            auto *mgr = reinterpret_cast<GlobalHotkeyManager*>(
                GetWindowLongPtr(hwnd, GWLP_USERDATA));

            if(mgr)
                mgr->handleHotkey(static_cast<int>(wp));

            return 0;
        }

        return DefWindowProc(hwnd, msg, wp, lp);
    }
}

GlobalHotkeyManager::GlobalHotkeyManager(){
    WNDCLASS wc = {};
    wc.lpfnWndProc = hotkeyWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "AudioMixerHotkeyWindow";

    RegisterClass(&wc);
    
    hwnd = CreateWindowEx(
        0, 
        wc.lpszClassName, 
        nullptr, 
        0,
        0, 
        0, 
        0, 
        0, 
        HWND_MESSAGE, 
        nullptr, 
        nullptr, 
        nullptr
    );

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    msgThread = std::thread([this]{
        MSG msg;
        
        while(GetMessage(&msg, nullptr, 0, 0) > 0)
            DispatchMessage(&msg);
    });
}

GlobalHotkeyManager::~GlobalHotkeyManager(){
    removeAll();

    if(hwnd){
        PostMessage(hwnd, WM_QUIT, 0, 0);
        hwnd = nullptr;
    }

    if(msgThread.joinable())
        msgThread.join();
}

void GlobalHotkeyManager::add(int modifiers, int virtualKey, Callback callback){
    const auto id = nextId++;

    if(RegisterHotKey(hwnd, id, modifiers, virtualKey))
        callbacks.emplace_back(id, std::move(callback));
}

void GlobalHotkeyManager::removeAll(){
    for(const auto &[id, _] : callbacks)
        UnregisterHotKey(hwnd, id);

    callbacks.clear();
}

void GlobalHotkeyManager::handleHotkey(int id){
    for(const auto &[storedId, callback] : callbacks){
        if(storedId == id && callback){
            juce::MessageManager::callAsync(callback);
            return;
        }
    }
}
