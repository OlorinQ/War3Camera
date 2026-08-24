#pragma once

#include "plugin.hpp"
#include <Windows.h>

// 窗口修复插件：按 END 键重置窗口，临时修复高分辨率下长时间游玩的字体重叠
class WindowFixer : public IPlugin
{
	public:
		virtual void Start();
		virtual void Stop(bool restore = true);

	private:
		static DWORD __stdcall Listen(LPVOID lpThreadParameter);
		HANDLE m_thread = NULL;
		bool m_running = false;
};
