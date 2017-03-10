#include "UnityPluginInterface.h"
#include "GfxDevice/d3d/GfxDeviceD3D9.h"

// --------------------------------------------------------------------------
// Allow writing to the Unity debug console from inside DLL land.
extern "C"
{
	void(_stdcall*debugLog)(char*) = nullptr;

	__declspec(dllexport) void LinkDebug(void(_stdcall*d)(char*))
	{
		debugLog = d;
	}

	__declspec(dllexport) void StartUp()
	{
		DebugLog("Plugin Start Up!");
		SetGfxDevice(new GfxDeviceD3D9());
	}
}

static inline void DebugLog(char* str)
{
#if _DEBUG
	if (debugLog) debugLog(str);
#endif
}

static void DoRender();

extern "C" void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{

}

extern "C" void EXPORT_API UnityRenderEvent(int eventID)
{
	if (eventID == 1)
		DoRender();
}

void DoRender()
{
	DebugLog("Do Native Render!");
}