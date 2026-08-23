#include "nowinkey.hpp"
#include <windows.h>

static HHOOK g_kbdHook = NULL;
static volatile bool g_running = false;

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION)
	{
		KBDLLHOOKSTRUCT* s = (KBDLLHOOKSTRUCT*)lParam;
		if (s && (s->vkCode == VK_LWIN || s->vkCode == VK_RWIN))
			return 1;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

static DWORD WINAPI HookThread(LPVOID)
{
	HMODULE mod = NULL;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&KeyboardProc, &mod);
	g_kbdHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, mod, 0);

	MSG msg;
	while (g_running && GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}

void NoWinKey::Start()
{
	g_running = true;
	m_thread = CreateThread(NULL, 0, HookThread, NULL, 0, NULL);
}

void NoWinKey::Stop()
{
	g_running = false;
	if (m_thread)
	{
		PostThreadMessageW(GetThreadId(m_thread), WM_NULL, 0, 0);
		if (WaitForSingleObject(m_thread, 100) == WAIT_TIMEOUT)
			TerminateThread(m_thread, 0);
		CloseHandle(m_thread);
		m_thread = NULL;
	}
	if (g_kbdHook)
	{
		UnhookWindowsHookEx(g_kbdHook);
		g_kbdHook = NULL;
	}
}