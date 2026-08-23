#include "helper.hpp"

#pragma comment(lib, "detours.lib")

static Helper* g_helper = NULL;

// DLL 入口：进程附加时加载插件，分离时卸载并释放
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	(void)lpReserved;

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// 游戏在 Miles 音频初始化时加载本 DLL；不接收线程附加通知，减少开销
			DisableThreadLibraryCalls(hModule);
			g_helper = new Helper();
			if (g_helper)
			{
				g_helper->LoadPlugins();
			}
			break;

		case DLL_PROCESS_DETACH:
			if (g_helper)
			{
				g_helper->UnloadPlugins();
				delete g_helper;
				g_helper = NULL;
			}
			break;
	}
	return TRUE;
}
