#include "borderless.hpp"
#include <windows.h>

// 去除窗口标题栏与可调边框的样式掩码（保留可见、最大化等）
#define BORDERLESS_STYLE_MASK 0xFF3BFFFF
#define WAR3_REG_KEY   "Software\\Blizzard Entertainment\\Warcraft III"
#define WAR3_REG_VALUE "Gfx Fullscreen"

// 强制游戏以窗口模式启动：把窗口化设置写入注册表（Storm.dll 启动时读取）。
// 值 1=全屏、0=窗口。若直接走独占全屏路径，Alt+Enter 会被游戏当作确认键（回车码 513 "OK"）从而破坏菜单导航。
static void ForceWindowedMode()
{
	HKEY hKey;
	if (RegCreateKeyExA(HKEY_CURRENT_USER, WAR3_REG_KEY, 0, NULL, 0,
		KEY_SET_VALUE, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return;

	DWORD value = 0;
	RegSetValueExA(hKey, WAR3_REG_VALUE, 0, REG_DWORD,
		reinterpret_cast<const BYTE*>(&value), sizeof(value));
	RegCloseKey(hKey);
}

// 去除标题栏并将窗口拉伸铺满屏幕（无边框全屏）
void BorderlessWindow::MakeBorderless()
{
	HWND hwnd = GetGameInstance()->GetGameWindow();
	if (!hwnd)
		return;

	DWORD currentStyle = GetWindowLong(hwnd, GWL_STYLE);
	DWORD newStyle = currentStyle & BORDERLESS_STYLE_MASK;
	newStyle |= WS_VISIBLE;

	SetWindowLong(hwnd, GWL_STYLE, newStyle);
	SetWindowLong(hwnd, GWL_EXSTYLE, 0);

	SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
				 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight,
				 SWP_FRAMECHANGED | SWP_NOACTIVATE);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// 校验样式是否生效，未生效则重试一次（游戏可能在 SetWindowLong 后被重置）
	DWORD verifyStyle = GetWindowLong(hwnd, GWL_STYLE);
	if (verifyStyle & WS_CAPTION)
	{
		SetWindowLong(hwnd, GWL_STYLE, newStyle);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	}
}

// 监视线程：持续检查窗口样式，带标题栏（窗口模式）时执行去边框
DWORD WINAPI BorderlessWindow::MonitorThread(LPVOID param)
{
	BorderlessWindow* pThis = (BorderlessWindow*)param;
	HWND hwnd = NULL;

	while (!hwnd && pThis->m_running)
	{
		hwnd = GetGameInstance()->GetGameWindow();
		if (!hwnd)
			Sleep(100);
	}

	if (!pThis->m_running)
		return 0;

	// 等待游戏初始化（可被停止打断，避免退出时卡耗时）
	for (int waited = 0; waited < 1500 && pThis->m_running; waited += 50)
		Sleep(50);
	if (!pThis->m_running)
		return 0;

	while (pThis->m_running)
	{
		// 窗口可能被游戏销毁重建（如过场视频全屏切换），失效时重新绑定主窗口
		if (!IsWindow(hwnd))
			hwnd = GetGameInstance()->GetGameWindow();

		// 窗口模式（有标题栏）下去边框并铺满屏；独占全屏下无标题栏则不动
		if (GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION)
			pThis->MakeBorderless();

		// 拆小睡眠，便于及时感知停止信号后尽快退出
		for (int waited = 0; waited < 500 && pThis->m_running; waited += 50)
			Sleep(50);
	}

	return 0;
}

void BorderlessWindow::Start()
{
	// 写窗口化注册表，让游戏以窗口起步（配合 MonitorThread 在窗口态下再去边框）
	ForceWindowedMode();

	m_running = true;
	m_thread = CreateThread(NULL, 0, MonitorThread, this, 0, NULL);
}

void BorderlessWindow::Stop(bool restore)
{
	(void)restore;
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
