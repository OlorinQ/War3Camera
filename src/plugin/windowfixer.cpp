#include "windowfixer.hpp"
#include "../game/warcraft.hpp"
#include <windows.h>

// 重置窗口：先将宽度 +1 再还原，强制游戏重建渲染缓冲，临时修复高分辨率下的字体重叠
static void ResetWindow(HWND target)
{
	if (!target)
		return;

	RECT rect;
	GetWindowRect(target, &rect);
	MoveWindow(target, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top, false);
	MoveWindow(target, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, false);
}

// 监听线程：检测 END 键，对鼠标所在的本进程窗口执行窗口重置
DWORD __stdcall WindowFixer::Listen(LPVOID lpThreadParameter)
{
	WindowFixer* pThis = (WindowFixer*)lpThreadParameter;
	POINT point;
	HWND target;
	DWORD origin_pid;
	DWORD target_pid;
	HWND org = GetGameInstance()->GetGameWindow();
	GetWindowThreadProcessId(org, &origin_pid);

	while (pThis->m_running)
	{
		if (System::IsKeyDown(VK_END))
		{
			GetCursorPos(&point);
			target = WindowFromPoint(point);
			if (target != NULL)
			{
				GetWindowThreadProcessId(target, &target_pid);
				if (origin_pid == target_pid)
					ResetWindow(target);
			}
		}
		Sleep(50);
	}
	return 0;
}

void WindowFixer::Start()
{
	m_running = true;
	m_thread = CreateThread(NULL, NULL, Listen, this, NULL, NULL);
}

void WindowFixer::Stop()
{
	m_running = false;
	if (m_thread)
	{
		// 有界等待，避免 DllMain 持 loader lock 时无限等待子线程造成死锁；
		// 超时则强制终止，确保退出不残留线程也不卡死
		if (WaitForSingleObject(m_thread, 100) == WAIT_TIMEOUT)
			TerminateThread(m_thread, 0);
		CloseHandle(m_thread);
		m_thread = NULL;
	}
}
