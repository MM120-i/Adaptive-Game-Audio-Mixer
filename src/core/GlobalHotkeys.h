#pragma once

#include <wtypes.h>
#include <thread>
#include <vector>
#include <mutex>
#include <utility>
#include <functional>

class GlobalHotkeyManager {
public:
    using Callback = std::function<void()>;

    static constexpr UINT WM_QUIT_THREAD = WM_APP + 2;

    GlobalHotkeyManager();
    ~GlobalHotkeyManager();

    bool add(UINT, UINT, Callback);
    void removeAll();
    void fireCombo(UINT mods, UINT vk);
    void checkHotkey(UINT vkCode);

private:
    struct Combo {
        UINT mods;
        UINT vk;
        Callback callback;
    };

    Callback findCallback(UINT, UINT) const;

    HHOOK keyboardHook_ = nullptr;
    HWND hwnd_ = nullptr;
    std::thread msgThread_;
    std::mutex callbackMutex_;
    std::vector<Combo> callbacks_;
};
