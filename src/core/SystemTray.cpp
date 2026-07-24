#include <future>
#include <windows.h>
#include <shellapi.h>

#include "SystemTray.h"

#define NOMINMAX

namespace {
    constexpr auto className = "AudioMixerTrayWindow";
    constexpr UINT trayId = 1;

    enum MenuIds {
        ID_SHOW = 1,
        ID_MUTE, 
        ID_QUIT,
    };
}

LRESULT CALLBACK SystemTray::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
    auto *self = reinterpret_cast<SystemTray *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg){
        case WM_TRAYICON:
            if(lp == WM_LBUTTONDBLCLK)
                if(self && self->onShow)
                    self->onShow();
            else if(lp == WM_RBUTTONUP)
                if(self)
                    self->showContextMenu();

            return 0;

        case WM_COMMAND:
            if(!self)
                return 0;
            
            switch (LOWORD(wp)){
                case ID_SHOW:
                    if(self->onShow)
                        self->onShow();
                    break;

                case ID_MUTE:
                    if(self->onMute)
                        self->onMute();
                    break;
                
                case ID_QUIT:
                    self->destroy();

                    if(self->onQuit)
                        self->onQuit();
                    break;
            }

            return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

SystemTray::SystemTray(){
    std::promise<void> ready;
    auto readyFuture = ready.get_future();

    msgThread = std::thread([this, &ready]{
        WNDCLASS wc = {};
        wc.lpfnWndProc = wndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = className;
        RegisterClass(&wc);

        hwnd = CreateWindowEx(0, className, nullptr, 0,
                              0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        ready.set_value();

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0) > 0)
            DispatchMessage(&msg);
    });

    readyFuture.wait();
}

SystemTray::~SystemTray(){
    destroy();

    if(hwnd){
        PostMessage(hwnd, WM_QUIT, 0, 0);
        hwnd = nullptr;
    }

    if(msgThread.joinable())
        msgThread.join();
}

void SystemTray::create(){
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = trayId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    strcpy_s(nid.szTip, _countof(nid.szTip), "AudioMixer");

    Shell_NotifyIcon(NIM_ADD, &nid);
    created = true;
}

void SystemTray::destroy(){
    if(!created)
        return;

    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = trayId;
    Shell_NotifyIcon(NIM_DELETE, &nid);
    created = false;
}

void SystemTray::showContextMenu(){
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    InsertMenuW(menu, 0, MF_BYPOSITION | MF_STRING, ID_SHOW, L"Show/Hide");
    InsertMenuW(menu, 1, MF_BYPOSITION | MF_STRING, ID_MUTE, muted_ ? L"Unmute" : L"Mute");
    InsertMenuW(menu, 2, MF_BYPOSITION | MF_STRING, ID_QUIT, L"Quit");
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

void SystemTray::updateMenuText(bool muted){
    muted_ = muted;
}