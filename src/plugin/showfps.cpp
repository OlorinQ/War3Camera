#include "showfps.hpp"
#include <windows.h>

void ShowFPS::Start()
{
	unsigned char bytes[] = { 0x01 };
	DWORD offset = GetGameInstance()->GetGameDllBase();
	Version version = GetGameInstance()->GetGameVersion();

	// FPS 显示标志字节偏移（RVA，相对 Game.dll 基址），从版本偏移表统一获取，
	// 经 IDA 逆向确认：该字节为游戏 HUD 帧率显示的开关（0=关闭，1=开启）
	const Offsets* offsets = GetOffsets(version);
	if (!offsets)
		return;

	offset += offsets->fpsShowFlag;

	// 保存原字节以便卸载时还原
	DWORD dwProtect;
	VirtualProtect((LPVOID)offset, 1, PAGE_EXECUTE_READWRITE, &dwProtect);
	m_patchByte = *(BYTE*)offset;
	VirtualProtect((LPVOID)offset, 1, dwProtect, &dwProtect);

	m_patchAddr = offset;
	m_patched = true;
	Game::PatchMemory(offset, bytes, 1);
}

void ShowFPS::Stop()
{
	if (m_patched && m_patchAddr)
	{
		unsigned char restore[] = { m_patchByte };
		Game::PatchMemory(m_patchAddr, restore, 1);
		m_patched = false;
		m_patchAddr = 0;
	}
}
