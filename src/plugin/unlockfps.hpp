#pragma once

#include "plugin.hpp"

// 帧率解锁插件：解除 60FPS 上限（通过补丁 + D3D9 呈现参数钩子），支持 Direct3D 与 OpenGL
class UnlockFPS : public IPlugin
{
	public:
		virtual void Start();
		virtual void Stop(bool restore = true);

	private:
		void ResetD3D();
		void ResetOpenGL();
		void WriteFPSLimit();

		DWORD m_patchAddr = 0;       // 帧率限制标志补丁地址（Game.dll 基址 + RVA）
		unsigned char m_patchByte = 0; // 补丁点原字节（卸载时还原）
		bool m_patched = false;      // 补丁是否已生效
		bool m_hooked = false;       // D3D9 呈现参数钩子是否已挂载
};
