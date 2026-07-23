#include "GlobalHotkeys.h"

#include <future>
#include <windows.h>
#include <juce_events/juce_events.h>

namespace {
    constexpr UINT WM_REGISTER_PENDING = WM_APP + 1;

    LRESULT CALLBACK hotkeyWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
        auto *mgr = reinterpret_cast<GlobalHotkeyManager*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if(msg == WM_HOTKEY){
            if(mgr)
                mgr->handleHotkey(static_cast<int>(wp));
            return 0;
        }

        if(msg == WM_REGISTER_PENDING && mgr){
            mgr->processPending();
            return 0;
        }

        return DefWindowProc(hwnd, msg, wp, lp);
    }
}

GlobalHotkeyManager::GlobalHotkeyManager(){
    std::promise<void> ready;
    auto readyFuture = ready.get_future();

    msgThread = std::thread([this, &ready]{
        WNDCLASS wc = {};
        wc.lpfnWndProc = hotkeyWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "AudioMixerHotkeyWindow";
        RegisterClass(&wc);

        hwnd = CreateWindowEx(
            0, wc.lpszClassName, nullptr, 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        ready.set_value();

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0) > 0)
            DispatchMessage(&msg);
    });

    readyFuture.wait();
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

    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbacks.emplace_back(id, std::move(callback));
    }

    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        pending.push_back({id, modifiers, virtualKey});
    }

    PostMessage(hwnd, WM_REGISTER_PENDING, 0, 0);
}

void GlobalHotkeyManager::processPending(){
    std::lock_guard<std::mutex> lock(pendingMutex);

    for(const auto &p : pending)
        RegisterHotKey(hwnd, p.id, p.mods, p.vk);

    pending.clear();
}

void GlobalHotkeyManager::removeAll(){
    std::lock_guard<std::mutex> lock(callbackMutex);

    for(const auto &[id, _] : callbacks)
        UnregisterHotKey(hwnd, id);

    callbacks.clear();
}

void GlobalHotkeyManager::handleHotkey(int id){
    std::lock_guard<std::mutex> lock(callbackMutex);

    for(const auto &[storedId, callback] : callbacks){
        if(storedId == id && callback){
            juce::MessageManager::callAsync(callback);
            return;
        }
    }
}
