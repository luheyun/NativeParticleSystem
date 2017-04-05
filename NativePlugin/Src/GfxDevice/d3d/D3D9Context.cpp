#include "PluginPrefix.h"
#include "D3D9Context.h"
#include "D3D9Enumeration.h"
#include "Shaders/GraphicsCaps.h"

static IDirect3D9*			s_D3D = NULL;
static IDirect3DDevice9*	s_Device = NULL;

static HINSTANCE			s_D3DDll = NULL;
static D3D9FormatCaps*		s_FormatCaps = NULL;

typedef IDirect3D9* (WINAPI* Direct3DCreate9Func)(UINT);
DWORD g_D3DAdapter = D3DADAPTER_DEFAULT;

bool InitializeD3D(D3DDEVTYPE devtype)
{
	GetGraphicsCaps().needsToSwizzleVertexColors = true;
	return true;
}

IDirect3DDevice9* GetD3DDevice()
{
	return s_Device;
}

void SetD3DDevice(IDirect3DDevice9* device, GfxDeviceEventType eventType)
{
	s_Device = device;

	// Create or release a small dynamic vertex buffer depending on the event type.
	//switch (eventType) {
	//case kGfxDeviceEventInitialize:
	//case kGfxDeviceEventAfterReset:
	//	// After device is initialized or was just reset, create the VB.
	//	if (!g_D3D9DynamicVB)
	//		g_D3D9Device->CreateVertexBuffer(1024, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &g_D3D9DynamicVB, NULL);
	//	break;
	//case kGfxDeviceEventBeforeReset:
	//case kGfxDeviceEventShutdown:
	//	// Before device is reset or being shut down, release the VB.
	//	SAFE_RELEASE(g_D3D9DynamicVB);
	//	break;
	//}
}
