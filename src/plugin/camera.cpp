#include "camera.hpp"
#include "../game/warcraft.hpp"
#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

bool CameraPlugin::s_running = false;
HANDLE CameraPlugin::s_threadHandle = NULL;
float* CameraPlugin::s_cameraDistanceAddress = NULL;
DWORD CameraPlugin::s_rvaOffset = 0;

CameraPlugin::~CameraPlugin()
{
	Stop();
}

void CameraPlugin::Start()
{
	if (s_running)
		return;

	s_running = true;
	s_cameraDistanceAddress = NULL;
	s_threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CameraDistanceThread, NULL, 0, NULL);
}

void CameraPlugin::Stop()
{
	s_running = false;

	if (s_threadHandle)
	{
		// 有界等待，避免 DllMain 持 loader lock 时无限等待子线程造成死锁；
		// 超时则强制终止，确保退出不残留线程也不卡死
		if (WaitForSingleObject(s_threadHandle, 100) == WAIT_TIMEOUT)
			TerminateThread(s_threadHandle, 0);
		CloseHandle(s_threadHandle);
		s_threadHandle = NULL;
	}
}

// 扫描 Game.dll 内存，定位镜头距离浮点常量的地址
// 特征值：1650.0f（默认视距）及其相邻的相机参数表
float* CameraPlugin::FindCameraDistanceAddress()
{
	Game* game = GetGameInstance();
	if (!game)
		return NULL;

	DWORD gameDllBase = game->GetGameDllBase();
	if (!gameDllBase)
		return NULL;

	// 先验证缓存的 RVA 是否仍有效，避免每次都全模块重扫（性能优化）
	if (s_rvaOffset)
	{
		DWORD* cached = (DWORD*)(gameDllBase + s_rvaOffset);
		if (IsDistanceSignature(cached, s_rvaOffset / sizeof(DWORD)))
			return (float*)cached;

		s_rvaOffset = 0;
	}

	MODULEINFO modInfo = {0};
	HMODULE hGameDll = GetModuleHandleA("Game.dll");
	if (!hGameDll)
		return NULL;

	if (!GetModuleInformation(GetCurrentProcess(), hGameDll, &modInfo, sizeof(modInfo)))
		return NULL;

	DWORD* scanStart = (DWORD*)gameDllBase;
	DWORD scanSize = (DWORD)(modInfo.SizeOfImage - 16);

	// 逐 DWORD 扫描，命中 1650.0f 后进一步校验相邻值确认特征
	for (DWORD i = 0; i < scanSize / sizeof(DWORD); i++)
	{
		DWORD* addr = scanStart + i;

		if (*addr != 0x44CE4000)
			continue;

		if (IsDistanceSignature(addr, i))
		{
			s_rvaOffset = (DWORD)((BYTE*)addr - (BYTE*)gameDllBase);
			return (float*)addr;
		}
	}

	return NULL;
}

// 校验 1650.0f 特征：相邻数据须符合相机参数表的任一排列
// 相关浮点常量：
//   0x44CE4000 = 1650.0f（默认镜头距离）
//   0x42C80000 = 100.0f（视距调节步长）
//   0x461C4000 = 10000.0f（视距范围上限）
//   0x44C80000 = 1600.0f、0x44BB8000 = 1500.0f、0x44AF0000 = 1400.0f（相邻预设视距）
bool CameraPlugin::IsDistanceSignature(DWORD* addr, DWORD index)
{
	if (addr[0] != 0x44CE4000)
		return false;

	// 1650.0f 后跟 0.0f / 100.0f / 10000.0f（相机参数表：默认距离 / 0 / 步长 / 上限）
	if (addr[1] == 0x0 && addr[2] == 0x42C80000 && addr[3] == 0x461C4000)
		return true;

	// 1650.0f 后跟 1600.0f / 1500.0f / 1400.0f（视距递减序列）
	if (addr[1] == 0x44C80000 && addr[2] == 0x44BB8000 && addr[3] == 0x44AF0000)
		return true;

	// 1650.0f 前有 1600.0f / 1500.0f / 1400.0f（视距递增序列）
	if (index >= 3 && addr[-1] == 0x44C80000 && addr[-2] == 0x44BB8000 && addr[-3] == 0x44AF0000)
		return true;

	return false;
}

// 相机距离监听线程：等待游戏启动后扫描地址，随后响应 +/-/* 键调整镜头距离
void CameraPlugin::CameraDistanceThread()
{
	// 等游戏启动；可被停止打断，避免退出时卡耗时
	for (int waited = 0; waited < 3000 && s_running; waited += 50)
		Sleep(50);
	if (!s_running)
		return;

	s_cameraDistanceAddress = FindCameraDistanceAddress();

	if (!s_cameraDistanceAddress)
		return;

	while (s_running)
	{
		Game* game = GetGameInstance();
		if (!game || !game->IsWarcraft())
		{
			Sleep(100);
			continue;
		}

		// 当前值异常（超出正常范围）时重新扫描，应对地址失效或数据被重置
		if (!s_cameraDistanceAddress || *s_cameraDistanceAddress < 100.0f || *s_cameraDistanceAddress > 5000.0f)
		{
			s_cameraDistanceAddress = FindCameraDistanceAddress();
			if (!s_cameraDistanceAddress)
			{
				Sleep(300);
				continue;
			}
		}

		// 放大视距（看得更近）：主键盘 + 或小键盘 +
		if (System::IsKeyDown(VK_ADD) || System::IsKeyDown(VK_OEM_PLUS))
		{
			AdjustCameraDistance(false);
			Sleep(200);
		}

		// 缩小视距（看得更远）：主键盘 - 或小键盘 -
		if (System::IsKeyDown(VK_SUBTRACT) || System::IsKeyDown(VK_OEM_MINUS))
		{
			AdjustCameraDistance(true);
			Sleep(200);
		}

		// 重置视距到默认 1650.0f：小键盘 *
		if (System::IsKeyDown(VK_MULTIPLY))
		{
			ResetCameraDistance();
			Sleep(200);
		}

		Sleep(10);
	}
}

// 按步长 100 调整镜头距离（zoomOut=true 表示拉远，即增大距离）
void CameraPlugin::AdjustCameraDistance(bool zoomOut)
{
	if (!s_cameraDistanceAddress)
		return;

	float currentValue = *s_cameraDistanceAddress;

	// 值异常时先重新定位地址
	if (currentValue < 100.0f || currentValue > 5000.0f)
	{
		s_cameraDistanceAddress = FindCameraDistanceAddress();
		if (!s_cameraDistanceAddress)
			return;
		currentValue = *s_cameraDistanceAddress;
	}

	float newValue;
	if (zoomOut)
	{
		newValue = currentValue + ADJUST_STEP;
		if (newValue > MAX_DISTANCE)
			newValue = MAX_DISTANCE;
	}
	else
	{
		newValue = currentValue - ADJUST_STEP;
		if (newValue < MIN_DISTANCE)
			newValue = MIN_DISTANCE;
	}

	if (newValue == currentValue)
		return;

	// 改写镜头距离（需临时放开页面保护）
	DWORD dwOldProtection;
	if (VirtualProtect((LPVOID)s_cameraDistanceAddress, sizeof(float), PAGE_EXECUTE_READWRITE, &dwOldProtection))
	{
		*s_cameraDistanceAddress = newValue;
		VirtualProtect((LPVOID)s_cameraDistanceAddress, sizeof(float), dwOldProtection, NULL);

		// 发送滚轮消息（PageUp/PageDown），让游戏相机立即读取新距离生效
		Game* game = GetGameInstance();
		if (game)
		{
			HWND hWnd = game->GetGameWindow();
			if (hWnd)
			{
				PostMessage(hWnd, WM_SYSKEYDOWN, VK_PRIOR, 0);
				PostMessage(hWnd, WM_SYSKEYDOWN, VK_NEXT, 0);
			}
		}
	}
}

// 重置镜头距离为默认值 1650.0f
void CameraPlugin::ResetCameraDistance()
{
	if (!s_cameraDistanceAddress)
		return;

	DWORD dwOldProtection;
	if (VirtualProtect((LPVOID)s_cameraDistanceAddress, sizeof(float), PAGE_EXECUTE_READWRITE, &dwOldProtection))
	{
		*s_cameraDistanceAddress = DEFAULT_DISTANCE;
		VirtualProtect((LPVOID)s_cameraDistanceAddress, sizeof(float), dwOldProtection, NULL);

		Game* game = GetGameInstance();
		if (game)
		{
			HWND hWnd = game->GetGameWindow();
			if (hWnd)
			{
				PostMessage(hWnd, WM_SYSKEYDOWN, VK_PRIOR, 0);
				PostMessage(hWnd, WM_SYSKEYDOWN, VK_NEXT, 0);
			}
		}
	}
}
