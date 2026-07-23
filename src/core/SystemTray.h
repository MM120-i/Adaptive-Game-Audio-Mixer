#pragma once

#include <functional>
#include <thread>
#include <windows.h>

class SystemTray {
private:
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

    void showContextMenu();

    HWND hwnd = nullptr;
    std::thread msgThread;
    bool created = false;
    bool muted_ = false;

    static constexpr UINT WM_TRAYICON = WM_APP + 1;

public:
    using Callback = std::function<void()>;

    SystemTray();
    ~SystemTray();

    void create();
    void destroy();
    void updateMenuText(bool);

    Callback onShow;
    Callback onQuit;
    Callback onMute;
};