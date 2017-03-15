#include "PluginPrefix.h"
#include "D3D9Context.h"
#include "D3D9Enumeration.h"

static IDirect3D9*			s_D3D = NULL;
static IDirect3DDevice9*	s_Device = NULL;

static HINSTANCE			s_D3DDll = NULL;
static D3D9FormatCaps*		s_FormatCaps = NULL;

typedef IDirect3D9* (WINAPI* Direct3DCreate9Func)(UINT);
DWORD g_D3DAdapter = D3DADAPTER_DEFAULT;

bool InitializeD3D(D3DDEVTYPE devtype)
{
	//g_D3DDevType = devtype;

	//s_D3DDll = LoadLibrary(L"d3d9.dll");
	//if (!s_D3DDll)
	//{
	//	//printf_console("d3d: no D3D9 installed\n");
	//	return false;
	//}

	//Direct3DCreate9Func createFunc = (Direct3DCreate9Func)GetProcAddress(s_D3DDll, "Direct3DCreate9");
	//if (!createFunc)
	//{
	//	//printf_console("d3d: Direct3DCreate9 not found\n");
	//	FreeLibrary(s_D3DDll);
	//	s_D3DDll = NULL;
	//	return false; // for some reason Direct3DCreate9 not found
	//}

	//// create D3D object
	//s_D3D = createFunc(D3D_SDK_VERSION);
	//if (!s_D3D)
	//{
	//	//printf_console("d3d: no 9.0c available\n");
	//	FreeLibrary(s_D3DDll);
	//	s_D3DDll = NULL;
	//	return false; // D3D initialization failed
	//}

	//// validate the adapter ordinal
	//UINT adapterCount = s_D3D->GetAdapterCount();
	//if (g_D3DAdapter >= adapterCount)
	//	g_D3DAdapter = D3DADAPTER_DEFAULT;

	//// check whether we have a HAL device
	//D3DDISPLAYMODE mode;
	//HRESULT hr;
	//if (FAILED(hr = s_D3D->GetAdapterDisplayMode(g_D3DAdapter, &mode)))
	//{
	//	//printf_console("d3d: failed to get adapter mode (adapter %d error 0x%08x)\n", g_D3DAdapter, hr);
	//	s_D3D->Release();
	//	s_D3D = NULL;
	//	FreeLibrary(s_D3DDll);
	//	s_D3DDll = NULL;
	//	return false; // failed to get adapter mode
	//}
	//if (FAILED(s_D3D->CheckDeviceType(g_D3DAdapter, g_D3DDevType, mode.Format, mode.Format, TRUE)))
	//{
	//	//printf_console("d3d: no support for this device type (accelerated/ref)\n");
	//	s_D3D->Release();
	//	s_D3D = NULL;
	//	FreeLibrary(s_D3DDll);
	//	s_D3DDll = NULL;
	//	return false; // no HAL driver available
	//}

	//// enumerate all formats, multi sample types and whatnot
	//s_FormatCaps = new D3D9FormatCaps();
	//if (!s_FormatCaps->Enumerate(*s_D3D))
	//{
	//	//printf_console("d3d: no video modes available\n");
	//	return false;
	//}

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
