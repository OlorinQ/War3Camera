#pragma once

#include <Windows.h>

// 支持的游戏版本枚举。
// 取值对应 Game.dll 文件版本号低 16 位：1.24e=6387、1.27a=52240
enum class Version : DWORD
{
	unknown = 0,
	v124e = 6387,
	v127a = 52240,
};
