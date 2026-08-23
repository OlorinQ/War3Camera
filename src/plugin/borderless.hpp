#pragma once

#include "plugin.hpp"
#include <Windows.h>

// 无边框全屏插件：强制游戏窗口化启动，随后去掉标题栏并铺满屏幕
class BorderlessWindow : public IPlugin
{
	public:
		virtual void Start();
		virtual void Stop();

	private:
		HANDLE m_thread = NULL;      // 监视线程句柄
		bool m_running = false;      // 运行标志

		void MakeBorderless();       // 去除标题栏并铺满屏幕
		static DWORD WINAPI MonitorThread(LPVOID param);
};
