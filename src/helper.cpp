#include "helper.hpp"
#include <windows.h>

// 游戏启动配置键：注册表根键（War3 视频/启动选项均在此）
#define WAR3_REG_KEY "Software\\Blizzard Entertainment\\Warcraft III"

// 写入 “Allow Local Files”=1：允许游戏加载本地自定义文件（mod），
// 配合 -loadfromarchive 参数即可启用地图/资源 mod
static void EnableLocalFiles()
{
	HKEY hKey;
	if (RegCreateKeyExA(HKEY_CURRENT_USER, WAR3_REG_KEY, 0, NULL, 0,
		KEY_SET_VALUE, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return;

	DWORD value = 1;
	RegSetValueExA(hKey, "Allow Local Files", 0, REG_DWORD,
		reinterpret_cast<const BYTE*>(&value), sizeof(value));
	RegCloseKey(hKey);
}

Helper::Helper()
	: m_plugins{
		new UnlockFPS(),        // 帧率解锁
		new ShowFPS(),          // FPS 显示
		new WindowFixer(),      // 字体重叠修复（END 键）
		new BorderlessWindow(), // 无边框全屏
		new NoWinKey(),         // 禁用 Win 键
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

	// 启用本地文件加载（Allow Local Files），方便启用 mod
	EnableLocalFiles();

	for (auto* p : m_plugins)
		p->Start();
}

void Helper::UnloadPlugins()
{
	for (auto* p : m_plugins)
		p->Stop();
}
