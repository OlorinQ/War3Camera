#pragma once

#include "plugin.hpp"
#include <Windows.h>

// 禁用 Win 键插件：游戏运行期间拦截左/右 Win 键，避免误触弹出开始菜单打扰游戏。
class NoWinKey : public IPlugin
{
	public:
		virtual void Start();
		virtual void Stop(bool restore = true);

	private:
		HANDLE m_thread = NULL;   // 钩子消息线程句柄
};