#ifndef IPLUGIN
#define IPLUGIN

#include "game/warcraft.hpp"

// 插件统一接口：每个功能模块实现 Start/Stop，由 Helper 统一管理生命周期
class IPlugin
{
	public:
		virtual ~IPlugin() {};

		virtual void Start() = 0;
		// restore=false 表示进程正在退出：仅停线程、不还原内存补丁，
		// 避免卸载时与游戏退出清理流程并发写内存引发崩溃
		virtual void Stop(bool restore = true) = 0;
};

#endif
