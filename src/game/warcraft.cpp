/**
 * 游戏接口基础函数：窗口查找、版本识别、内存补丁与 Detours 钩子
 */

#include "warcraft.hpp"

#include <detours.h>
#include <utility>

// 文件版本信息 API（GetFileVersionInfoA 等）所在库
#pragma comment(lib, "Version.lib")

// 1.24e / 1.27a 的逆向外挂偏移表（RVA），经 IDA 逆向确认
static const Offsets kOffsetsV124e = {
	0x5720,   // getGameOpt      : 游戏偏好设置单例获取函数
	0x57F0,   // setGameOptValue : 游戏偏好选项写入函数
	0x62DF9B, // fpsLimitPatch   : 帧率限制标志补丁点
	0x62E1E0, // d3d9Present     : D3D9 呈现参数获取函数
	0xA9E764, // gxType          : 图形渲染后端类型全局变量
	0x34B248, // fpsShowFlag     : FPS 显示标志字节
};

static const Offsets kOffsetsV127a = {
	0x23E00,  // getGameOpt
	0x25A70,  // setGameOptValue
	0x5FCFB,  // fpsLimitPatch
	0xEC6B0,  // d3d9Present
	0xB665C8, // gxType
	0x397E5A, // fpsShowFlag
};

// 获取指定版本的偏移表；版本不受支持时返回 NULL
const Offsets* GetOffsets(Version version)
{
	switch (version)
	{
		case Version::v124e: return &kOffsetsV124e;
		case Version::v127a: return &kOffsetsV127a;
		default: return NULL;
	}
}

// 查找指定进程最顶层的无属主窗口（即游戏主窗口）
static HWND findTopWindow(DWORD pid)
{
	std::pair<HWND, DWORD> params = {0, pid};

	BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
	{
		auto pParams = (std::pair<HWND, DWORD>*)(lParam);

		DWORD processId;
		if (GetWindowThreadProcessId(hwnd, &processId) &&
		    processId == pParams->second &&
		    GetWindow(hwnd, GW_OWNER) == 0)
		{
			SetLastError((DWORD)-1);
			pParams->first = hwnd;
			return FALSE;
		}
		return TRUE;
	}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
	{
		return params.first;
	}
	return 0;
}

Game* GetGameInstance()
{
	static auto game = new Game();
	return game;
}

Game::Game()
{
	UINT bytesCount = 0;
	char data[2048] = {0};
	VS_FIXEDFILEINFO *fileInfo = NULL;

	// 读取 Game.dll 的文件版本号，低 16 位即版本号（如 1.24e=6387、1.27a=52240）
	if (GetFileVersionInfoA("Game.dll", 0, sizeof(data), data) &&
		VerQueryValueA(data, "\\", (void**)&fileInfo, &bytesCount) && fileInfo)
	{
		m_war3Version = static_cast<Version>(fileInfo->dwFileVersionLS & 0xffff);
	}

	m_gamedllBase = (DWORD)GetModuleHandle("Game.dll");
}

Game::~Game() {}

// 内联钩子：用 Detours 把目标函数前几条指令替换为跳转，pCallBackFuncAddr 返回原函数真实入口
bool Game::InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void *&pCallBackFuncAddr)
{
	if (!pOldFuncAddr) return false;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(void *&)pOldFuncAddr, pNewFuncAddr);
	if (DetourTransactionCommit() != 0)
		return false;
	pCallBackFuncAddr = pOldFuncAddr;
	return true;
}

// 撤销内联钩子，恢复原函数
bool Game::DetachHook(void *&pOldFuncAddr, void *pNewFuncAddr)
{
	if (!pOldFuncAddr) return false;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&pOldFuncAddr, pNewFuncAddr);
	return DetourTransactionCommit() == 0;
}

// 改写游戏进程内存：先改页面保护再写入，最后恢复原保护属性
void Game::PatchMemory(uintptr_t pAddress, unsigned char *bytes, uint32_t size)
{
	if (!pAddress) return;

	unsigned long Protection;
	void *address = reinterpret_cast<void *>(pAddress);

	VirtualProtect(address, size, PAGE_READWRITE, &Protection);
	memcpy(address, (const void *)bytes, size);
	VirtualProtect(address, size, Protection, &Protection);
}

HWND Game::GetGameWindow()
{
	// 窗口可能被游戏销毁重建（如过场视频全屏切换），句柄失效时自动重新查找
	if (!m_hWnd || !IsWindow(m_hWnd))
	{
		m_hWnd = findTopWindow(GetCurrentProcessId());
	}
	return m_hWnd;
}

DWORD Game::GetGameDllBase()
{
	return m_gamedllBase;
}

Version Game::GetGameVersion()
{
	return m_war3Version;
}

// 判断当前进程是否为魔兽争霸（检查主程序文件名是否为 war3.exe）
bool Game::IsWarcraft()
{
	char path[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, path, MAX_PATH);

	int len = strlen(path);
	for (int i = 0; i < len; i++)
	{
		if (path[i] >= 'A' && path[i] <= 'Z')
			path[i] += 'a' - 'A';
	}

	return strstr(path, "war3.exe") != NULL;
}
