#pragma once

#include "plugin.hpp"

// 视距调整插件：通过内存特征扫描定位镜头距离参数，响应 +/-/* 键调整
class CameraPlugin : public IPlugin
{
public:
	~CameraPlugin();

	void Start() override;
	void Stop() override;

private:
	static void CameraDistanceThread();
	static void AdjustCameraDistance(bool zoomOut);
	static void ResetCameraDistance();
	static float* FindCameraDistanceAddress();
	static bool IsDistanceSignature(DWORD* addr, DWORD index);

	static bool s_running;
	static HANDLE s_threadHandle;
	static float* s_cameraDistanceAddress; // 镜头距离参数地址（Game.dll 内）
	static DWORD s_rvaOffset;              // 镜头距离参数相对 Game.dll 基址的 RVA（缓存避免重扫）

	// 镜头距离调节范围与步长
	static constexpr float MIN_DISTANCE = 250.0f;
	static constexpr float MAX_DISTANCE = 3050.0f;
	static constexpr float DEFAULT_DISTANCE = 1650.0f;
	static constexpr float ADJUST_STEP = 100.0f;
};
