#pragma once
#include "stdafx.h"
#include "BaseHelper.h"
#ifdef _WINDOWS
#include <thread>
#include <atomic>
#include <future>
#endif //_WINDOWS
class ClipboardControl : public BaseHelper
{
#ifdef _WINDOWS
private:
    // Мониторинг крутится в собственном потоке с message pump — иначе WM_CLIPBOARDUPDATE
    // не доставляется (у потока 1С своего цикла сообщений для нашего окна нет).
    std::thread monitorThread;
    std::atomic<HWND> hClipboardMonitor{nullptr};
    std::atomic<DWORD> monitorThreadId{0};
    void SetMonitoring(bool value);
    bool GetMonitoring();
    void MonitorThreadProc(std::promise<bool> ready);
public:
    void SendEvent();
#endif //_WINDOWS
private:
    static std::vector<std::u16string> names;
    ClipboardControl();
    virtual ~ClipboardControl();
};
