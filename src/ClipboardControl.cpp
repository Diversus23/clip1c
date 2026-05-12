#include "stdafx.h"
#include "ClipboardControl.h"
#include "ClipboardManager.h"

std::vector<std::u16string> ClipboardControl::names = {
	AddComponent(u"ClipboardControl", []() { return new ClipboardControl; }),
};

ClipboardControl::ClipboardControl()
{
	AddProperty(u"Text", u"Текст",
		[&](VH var) { var = ClipboardManager().GetText(); },
		[&](VH var) { ClipboardManager().SetText(var); }
	);
	AddProperty(u"Files", u"Файлы", 
		[&](VH var) { var = ClipboardManager().GetFiles(); },
		[&](VH var) { ClipboardManager().SetFiles(var); }
	);
	AddProperty(u"Image", u"Картинка", 
		[&](VH var) { ClipboardManager().GetImage(var); },
		[&](VH var) { ClipboardManager().SetImage(var); }
	);
	AddProperty(u"Format", u"Формат",
		[&](VH var) { var = ClipboardManager().GetFormat(); }
	);

#ifdef _WINDOWS
	AddProperty(u"Monitoring", u"Мониторинг",
		[&](VH value) { value = this->GetMonitoring(); },
		[&](VH value) { this->SetMonitoring(value); }
	);
#endif //_WINDOWS
	
	AddFunction(u"Empty", u"Очистить", [&]() { this->result = ClipboardManager().Empty(); });
	AddFunction(u"SetText", u"ЗаписатьТекст", [&](VH var) { this->result = ClipboardManager().SetText(var); });
	AddFunction(u"SetFiles", u"ЗаписатьФайлы", [&](VH var) { this->result = ClipboardManager().SetFiles(var); });
	AddFunction(u"SetImage", u"ЗаписатьКартинку", [&](VH var) { this->result = ClipboardManager().SetImage(var); });
}

ClipboardControl::~ClipboardControl()
{
#ifdef _WINDOWS
	SetMonitoring(false);
#endif //_WINDOWS
}

#ifdef _WINDOWS

#include <atomic>

void ClipboardControl::SendEvent()
{
	ClipboardManager manager;
	std::u16string format = MB2WCHAR(manager.GetFormat());
	std::u16string text = MB2WCHAR(WC2MB(manager.GetText()));
	ExternalEvent(format, text);
}

static LRESULT CALLBACK ClipboardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLIPBOARDUPDATE: {
		auto component = (ClipboardControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (component) component->SendEvent();
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

bool ClipboardControl::GetMonitoring()
{
	return hClipboardMonitor.load() != nullptr;
}

void ClipboardControl::MonitorThreadProc(std::promise<bool> ready)
{
	// Регистрация класса один раз на DLL. Повторный RegisterClass с тем же hInstance
	// вернёт 0 и SetLastError(ERROR_CLASS_ALREADY_EXISTS) — это нормально, окно создастся
	// по уже зарегистрированному классу.
	const LPCWSTR wsClassName = L"VanessaClipboardMonitor";
	WNDCLASS wndClass = {};
	wndClass.hInstance = hModule;
	wndClass.lpszClassName = wsClassName;
	wndClass.lpfnWndProc = ClipboardWndProc;
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowW(wsClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
	if (!hwnd) {
		ready.set_value(false);
		return;
	}
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
	if (!AddClipboardFormatListener(hwnd)) {
		DestroyWindow(hwnd);
		ready.set_value(false);
		return;
	}
	hClipboardMonitor.store(hwnd);
	monitorThreadId.store(GetCurrentThreadId());
	// Сигналим успешный старт ДО входа в message pump — основной поток разблокируется.
	ready.set_value(true);

	// Стандартный message pump. Выход — по WM_QUIT (PostThreadMessage из SetMonitoring(false)).
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	RemoveClipboardFormatListener(hwnd);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
	DestroyWindow(hwnd);
	hClipboardMonitor.store(nullptr);
	monitorThreadId.store(0);
}

void ClipboardControl::SetMonitoring(bool value)
{
	bool running = (hClipboardMonitor.load() != nullptr);
	if (value == running) return;

	if (value) {
		// Подчищаем joinable-состояние от предыдущей попытки старта, если та упала.
		if (monitorThread.joinable()) monitorThread.join();

		std::promise<bool> ready;
		auto fut = ready.get_future();
		monitorThread = std::thread(&ClipboardControl::MonitorThreadProc, this, std::move(ready));
		bool ok = fut.get();
		if (!ok) {
			// Поток уже вышел из MonitorThreadProc — корректно join-им, чтобы не оставлять joinable-state,
			// иначе следующий SetMonitoring(true) увидел бы fake-running и тихо завис.
			if (monitorThread.joinable()) monitorThread.join();
		}
	}
	else {
		DWORD tid = monitorThreadId.load();
		if (tid) PostThreadMessage(tid, WM_QUIT, 0, 0);
		if (monitorThread.joinable()) monitorThread.join();
	}
}

#endif //_WINDOWS
