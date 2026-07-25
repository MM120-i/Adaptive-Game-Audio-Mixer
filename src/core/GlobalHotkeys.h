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

    GlobalHotkeyManager();
    ~GlobalHotkeyManager();

    bool add(int, int, Callback);
    void removeAll();
    void handleHotkey(int);
    void processPending();

private:
    HWND hwnd = nullptr;
    std::thread msgThread;
    std::mutex callbackMutex;
    std::vector<std::pair<int, Callback>> callbacks;
    std::mutex pendingMutex;
    struct Pending { int id; int mods; int vk; };
    std::vector<Pending> pending;
    int nextId = 1;
};