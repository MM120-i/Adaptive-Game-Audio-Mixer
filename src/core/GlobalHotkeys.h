#pragma once

#include <wtypes.h>
#include <thread>
#include <vector>
#include <utility>
#include <functional>

class GlobalHotkeyManager {
public:
    using Callback = std::function<void()>;

    GlobalHotkeyManager();
    ~GlobalHotkeyManager();

    void add(int, int, Callback);
    void removeAll();
    void handleHotkey(int);

private:
    HWND hwnd = nullptr;
    std::thread msgThread;
    std::vector<std::pair<int, Callback>> callbacks;
    int nextId = 1;
};