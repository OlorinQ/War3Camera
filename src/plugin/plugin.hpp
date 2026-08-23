#ifndef IPLUGIN
#define IPLUGIN

#include "game/warcraft.hpp"

// 插件统一接口：每个功能模块实现 Start/Stop，由 Helper 统一管理生命周期
class IPlugin
{
	public:
		virtual ~IPlugin() {};

		virtual void Start() = 0;
		virtual void Stop() = 0;
};

#endif
