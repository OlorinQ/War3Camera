#pragma once

#include "version.hpp"
#include <stdint.h>

// 系统级辅助（静态工具）
class System
{
	public:
		// 判断按键当前是否处于按下状态（GetAsyncKeyState 高位）
		static BOOL IsKeyDown(int key)
		{
			short state = GetAsyncKeyState(key);
			return (state & 0x8000) ? true : false;
		}
};

// 各功能逆向外挂偏移表（RVA，相对 Game.dll 基址），按版本集中管理
struct Offsets
{
	DWORD getGameOpt;      // 游戏偏好设置单例获取函数
	DWORD setGameOptValue; // 游戏偏好选项写入函数
	DWORD fpsLimitPatch;   // 帧率限制标志补丁点
	DWORD d3d9Present;     // D3D9 呈现参数获取函数（钩子点）
	DWORD gxType;          // 图形渲染后端类型全局变量
	DWORD fpsShowFlag;     // FPS 显示标志字节
};

// 获取指定版本的偏移表；版本不受支持时返回 NULL
const Offsets* GetOffsets(Version version);

// 游戏实例抽象：提供版本识别、窗口句柄、模块基址、内存补丁与钩子等能力
class Game
{
	public:
		Game();
		~Game();

		static bool InlineHook(void *pOldFuncAddr, void *pNewFuncAddr, void *&pCallBackFuncAddr);
		static bool DetachHook(void *&pOldFuncAddr, void *pNewFuncAddr);
		static void PatchMemory(uintptr_t pAddress, unsigned char *bytes, uint32_t size);

		HWND GetGameWindow();
		Version GetGameVersion();
		DWORD GetGameDllBase();
		bool IsWarcraft();

	private:
		HWND m_hWnd{NULL};              // 游戏主窗口句柄（失效时自动重查）
		DWORD m_gamedllBase{NULL};      // Game.dll 模块基址
		Version m_war3Version{Version::unknown}; // 识别到的游戏版本
};

Game* GetGameInstance();
