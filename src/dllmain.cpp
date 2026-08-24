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
				// lpReserved != NULL 表示进程正在退出：只停线程、不还原内存补丁，
				// 避免卸载时与游戏退出清理流程并发写内存引发崩溃
				g_helper->UnloadPlugins(lpReserved != NULL);
				delete g_helper;
				g_helper = NULL;
			}
			break;
	}
	return TRUE;
}
