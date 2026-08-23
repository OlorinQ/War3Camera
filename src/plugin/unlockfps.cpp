#include "unlockfps.hpp"
#include <windows.h>
#include <d3d9.h>

// 游戏图形渲染后端类型（Gx 全局变量取值）
enum GxType
{
	GxType_Direct3D = 0,
	GxType_OpenGL = 1
};

// 游戏偏好设置单例获取函数（返回 CPreferencesWar3 对象指针）
static DWORD(*GetGameOpt)() = 0;
// 游戏偏好选项写入函数（按索引写入，索引需 < 79）
static DWORD(__fastcall* SetGameOptValue)(DWORD pthis, DWORD unused, DWORD idx, DWORD value) = 0;

// 被钩子的原函数：D3D9 呈现参数获取函数（GetPresentParameters）
static DWORD(__fastcall* org_GetD3d9Parameters)(DWORD pthis, DWORD unused, D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;

// 钩子回调：获取呈现参数后强制改为“立即呈现”，从而关闭垂直同步、解锁帧率
static DWORD __fastcall GetD3d9Parameters(DWORD pthis, DWORD unused, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	DWORD result = org_GetD3d9Parameters(pthis, unused, pPresentationParameters);
	if (pPresentationParameters)
		pPresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	return result;
}

void UnlockFPS::Start()
{
	DWORD base = GetGameInstance()->GetGameDllBase();
	DWORD patchAddr = 0;
	DWORD d3d9Addr = 0;
	DWORD *gxTypeAddr = 0;
	Version version = GetGameInstance()->GetGameVersion();

	// 统一从版本偏移表取各函数/全局变量地址（RVA，经 IDA 逆向确认）
	const Offsets* offsets = GetOffsets(version);
	if (!offsets)
		return;

	GetGameOpt = (DWORD(*)())(base + offsets->getGameOpt);        // 偏好单例获取函数
	SetGameOptValue = (DWORD(__fastcall*)(DWORD, DWORD, DWORD, DWORD))(base + offsets->setGameOptValue); // 偏好选项写入函数
	patchAddr = base + offsets->fpsLimitPatch;                    // 帧率限制标志补丁点
	d3d9Addr = base + offsets->d3d9Present;                       // D3D9 呈现参数获取函数
	gxTypeAddr = (DWORD*)(base + offsets->gxType);                // 图形后端类型全局变量

	WriteFPSLimit();

	m_patchAddr = patchAddr;

	if (patchAddr)
	{
		// 保存原字节以便卸载时还原
		DWORD dwProtect;
		VirtualProtect((LPVOID)patchAddr, 1, PAGE_EXECUTE_READWRITE, &dwProtect);
		m_patchByte = *(BYTE*)patchAddr;
		VirtualProtect((LPVOID)patchAddr, 1, dwProtect, &dwProtect);

		unsigned char bytes[] = { 0xFF };
		Game::PatchMemory(patchAddr, bytes, 1);
		m_patched = true;
	}

	if (d3d9Addr)
		m_hooked = Game::InlineHook((void*)d3d9Addr, GetD3d9Parameters, (void*&)org_GetD3d9Parameters);

	if (!gxTypeAddr)
		return;

	// 独占全屏（窗口无标题栏）下不做最小化重建交换链，否则会黑屏/卡死（常见于 KK 等平台以全屏启动）。
	// 此时只保留 patch + presenter 钩子，即“仅显示 FPS 不解锁”，避免与独占全屏冲突。
	{
		HWND wnd = GetGameInstance()->GetGameWindow();
		bool exclusiveFullscreen = (wnd != NULL && (GetWindowLong(wnd, GWL_STYLE) & WS_CAPTION) == 0);

		if (!exclusiveFullscreen)
		{
			if (*gxTypeAddr == GxType_Direct3D)
				ResetD3D();
			else if (*gxTypeAddr == GxType_OpenGL)
				ResetOpenGL();
		}
	}
}

// 重置 D3D9 渲染链：通过最小化再还原窗口，强制游戏重建交换链以套用新的垂直同步设置
void UnlockFPS::ResetD3D()
{
	HWND target = GetGameInstance()->GetGameWindow();
	ShowWindow(target, SW_MINIMIZE);
	ShowWindow(target, SW_SHOWNORMAL);
}

// 重置 OpenGL 渲染：直接关闭垂直同步间隔（wglSwapIntervalEXT(0)）
void UnlockFPS::ResetOpenGL()
{
	auto wglSwapIntervalEXT = (DWORD(*)(DWORD))wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(0);
}

// 将游戏刷新率上限写入与显示器刷新率一致（>60Hz 时生效）
void UnlockFPS::WriteFPSLimit()
{
	DEVMODE dm;
	memset(&dm, 0, sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &dm);

	if (dm.dmDisplayFrequency > 60 && GetGameOpt && SetGameOptValue)
		SetGameOptValue(GetGameOpt(), 0, 4, dm.dmDisplayFrequency);
}

void UnlockFPS::Stop()
{
	// 还原帧率限制标志补丁
	if (m_patched && m_patchAddr)
	{
		unsigned char restore[] = { m_patchByte };
		Game::PatchMemory(m_patchAddr, restore, 1);
		m_patched = false;
	}

	// 撤销 D3D9 呈现参数钩子
	if (m_hooked && org_GetD3d9Parameters)
	{
		Game::DetachHook((void*&)org_GetD3d9Parameters, GetD3d9Parameters);
		m_hooked = false;
	}
}