#pragma once

#include "plugin.hpp"

// FPS 显示插件：将游戏 HUD 帧率显示标志字节置 1，显示当前帧率
class ShowFPS : public IPlugin
{
	public:
		void Start();
		void Stop();

	private:
		DWORD m_patchAddr = 0;       // 显示标志字节地址（Game.dll 基址 + RVA）
		unsigned char m_patchByte = 0; // 原字节（卸载时还原）
		bool m_patched = false;      // 补丁是否已生效
};
