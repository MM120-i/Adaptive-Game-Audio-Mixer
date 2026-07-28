#include "GlobalHotkeys.h"

#include <algorithm>
#include <juce_events/juce_events.h>

#define NOMINMAX

namespace {
    GlobalHotkeyManager *g_mgr = nullptr;

    LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam){
        if(nCode == HC_ACTION && g_mgr){
            auto *kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

            if(wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                g_mgr->checkHotkey(static_cast<UINT>(kbd->vkCode));
        }

        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    LRESULT CALLBACK hiddenWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
        if(msg == GlobalHotkeyManager::WM_QUIT_THREAD){
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, msg, wp, lp);
    }

    UINT buildModifiers(){
        UINT mods = 0;

        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
            mods |= MOD_CONTROL;

        if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
            mods |= MOD_SHIFT;

        if(GetAsyncKeyState(VK_MENU) & 0x8000)
            mods |= MOD_ALT;

        if(GetAsyncKeyState(VK_LWIN) & 0x8000 || GetAsyncKeyState(VK_RWIN) & 0x8000)
            mods |= MOD_WIN;

        return mods;
    }
}

GlobalHotkeyManager::GlobalHotkeyManager(){
    std::promise<void> ready;
    auto readyFuture = ready.get_future();

    msgThread_ = std::thread([this, &ready]{
        WNDCLASS wc = {};
        wc.lpfnWndProc = hiddenWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "AudioMixerHotkeyWindow";
        RegisterClass(&wc);

        hwnd_ = CreateWindowEx(
            0, wc.lpszClassName, nullptr, 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr
        );

        keyboardHook_ = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProc, nullptr, 0);

        if(!hwnd_ || !keyboardHook_){
            if(keyboardHook_){
                UnhookWindowsHookEx(keyboardHook_);
                keyboardHook_ = nullptr;
            }

            if(hwnd_){
                DestroyWindow(hwnd_);
                hwnd_ = nullptr;
            }

            ready.set_value();

            return;
        }

        g_mgr = this;
        ready.set_value();

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0) > 0){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(g_mgr == this)
            g_mgr = nullptr;

        UnhookWindowsHookEx(keyboardHook_);
        keyboardHook_ = nullptr;
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    });

    readyFuture.wait();
}

GlobalHotkeyManager::~GlobalHotkeyManager(){
    removeAll();

    if(hwnd_)
        PostMessage(hwnd_, WM_QUIT_THREAD, 0, 0);

    if(msgThread_.joinable())
        msgThread_.join();
}

bool GlobalHotkeyManager::add(UINT mods, UINT vk, Callback callback){
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callbacks_.push_back({mods, vk, std::move(callback)});

    return true;
}

void GlobalHotkeyManager::removeAll(){
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callbacks_.clear();
}

void GlobalHotkeyManager::checkHotkey(UINT vkCode){
    UINT mods = buildModifiers();
    Callback cb;

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        cb = findCallback(mods, vkCode);
    }

    if(cb)
        juce::MessageManager::callAsync(std::move(cb));
}

void GlobalHotkeyManager::fireCombo(UINT mods, UINT vk){
    Callback cb;

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        cb = findCallback(mods, vk);
    }

    if(cb)
        cb();
}

GlobalHotkeyManager::Callback GlobalHotkeyManager::findCallback(UINT mods, UINT vk) const {
    auto it = std::find_if(callbacks_.begin(), callbacks_.end(),
        [mods, vk](const Combo &c){ return c.mods == mods && c.vk == vk && c.callback; });

    return it != callbacks_.end() ? it->callback : nullptr;
}
