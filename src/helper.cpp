#include "helper.hpp"
#include <windows.h>

Helper::Helper()
	: m_plugins{
		new CameraPlugin(),     // 视距调整
	}
{
}

Helper::~Helper()
{
	for (auto* p : m_plugins)
		delete p;
}

void Helper::LoadPlugins()
{
	auto game = GetGameInstance();

	// 仅在魔兽争霸进程内且 Game.dll 已加载时启动，避免误注入其他进程
	if (game == NULL || !game->IsWarcraft())
		return;

	if (!game->GetGameDllBase())
		return;

	for (auto* p : m_plugins)
		p->Start();
}

void Helper::UnloadPlugins(bool exiting)
{
	// exiting=true（进程退出）：仅停线程不还原补丁，避免卸载时与游戏退出流程竞争写内存
	for (auto* p : m_plugins)
		p->Stop(!exiting);
}
