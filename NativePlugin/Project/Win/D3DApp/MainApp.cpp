#include "Include/Precomplie/PluginPrefix.h"
#include<d3d9.h>   
#include <math.h>
#include <tchar.h>   
#include "D3D9Enumeration.h"
#include "GfxDevice/d3d/D3D9Context.h"
#include "GfxDevice/GfxDevice.h"
#include "GfxDevice/d3d/GfxDeviceD3D9.h"
#include "Graphics/Mesh/MeshVertexFormat.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "GfxDevice/ChannelAssigns.h"

#pragma comment(lib, "d3d9.lib")   

#define WINDOW_CLASS _T("UGPDX")   
#define WINDOW_NAME  _T("Blank D3D Window")   


// Function Prototypes...   
bool InitializeD3D(HWND hWnd, bool fullscreen);
void RenderScene();
void Shutdown();


// Direct3D object and device.   
LPDIRECT3D9 g_D3D = NULL;
LPDIRECT3DDEVICE9 g_D3DDevice = NULL;


LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;

	case WM_KEYUP:
		if (wParam == VK_ESCAPE) PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE prevhInst, LPSTR cmdLine, int show)
{
	// Register the window class   
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		WINDOW_CLASS, NULL };
	RegisterClassEx(&wc);

	// Create the application's window   
	HWND hWnd = CreateWindow(WINDOW_CLASS, WINDOW_NAME, WS_OVERLAPPEDWINDOW,
		100, 100, 640, 480, GetDesktopWindow(), NULL,
		wc.hInstance, NULL);

	// Initialize Direct3D   
	if (InitializeD3D(hWnd, false))
	{
		// Show the window   
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);

		// Enter the message loop   
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));

		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				RenderScene();
		}
	}

	// Release any and all resources.   
	Shutdown();

	// Unregister our window.   
	UnregisterClass(WINDOW_CLASS, wc.hInstance);
	return 0;
}

static IDirect3DVertexBuffer9* g_D3D9DynamicVB;
static IDirect3D9*			s_D3D = NULL;
static HINSTANCE			s_D3DDll = NULL;
typedef IDirect3D9* (WINAPI* Direct3DCreate9Func)(UINT);
DWORD g_D3DAdapter = D3DADAPTER_DEFAULT;
D3DDEVTYPE g_D3DDevType = D3DDEVTYPE_REF;
D3DCAPS9	s_d3dcaps;
static D3D9FormatCaps*		s_FormatCaps = NULL;

bool InitializeD3D(HWND hWnd, bool fullscreen)
{
	D3DDEVTYPE devtype = D3DDEVTYPE_REF;
	s_D3DDll = LoadLibrary(L"d3d9.dll");
	if (!s_D3DDll)
	{
		return false; // no d3d9 installed
	}

	Direct3DCreate9Func createFunc = (Direct3DCreate9Func)GetProcAddress(s_D3DDll, "Direct3DCreate9");
	if (!createFunc)
	{
		FreeLibrary(s_D3DDll);
		s_D3DDll = NULL;
		return false; // for some reason Direct3DCreate9 not found
	}

	// create D3D object
	s_D3D = createFunc(D3D_SDK_VERSION);
	if (!s_D3D)
	{
		FreeLibrary(s_D3DDll);
		s_D3DDll = NULL;
		return false; // D3D initialization failed
	}

	// validate the adapter ordinal
	UINT adapterCount = s_D3D->GetAdapterCount();
	if (g_D3DAdapter >= adapterCount)
		g_D3DAdapter = D3DADAPTER_DEFAULT;

	// check whether we have a HAL device
	D3DDISPLAYMODE mode;
	HRESULT hr;
	if (FAILED(hr = s_D3D->GetAdapterDisplayMode(g_D3DAdapter, &mode)))
	{
		s_D3D->Release();
		s_D3D = NULL;
		FreeLibrary(s_D3DDll);
		s_D3DDll = NULL;
		return false; // failed to get adapter mode
	}

	if (FAILED(s_D3D->CheckDeviceType(g_D3DAdapter, g_D3DDevType, mode.Format, mode.Format, TRUE)))
	{
		s_D3D->Release();
		s_D3D = NULL;
		FreeLibrary(s_D3DDll);
		s_D3DDll = NULL;
		return false; // no HAL driver available
	}

	// enumerate all formats, multi sample types and whatnot
	s_FormatCaps = new D3D9FormatCaps();
	if (!s_FormatCaps->Enumerate(*s_D3D))
	{
		return false;
	}


	D3DPRESENT_PARAMETERS pparams;
	ZeroMemory(&pparams, sizeof(D3DPRESENT_PARAMETERS));
	pparams.BackBufferWidth = 1024;
	pparams.BackBufferHeight = 860;
	pparams.BackBufferCount = 1;
	pparams.hDeviceWindow = hWnd;
	pparams.EnableAutoDepthStencil = FALSE;
	pparams.Windowed = fullscreen ? FALSE : TRUE;
	pparams.SwapEffect = D3DSWAPEFFECT_DISCARD;

	bool deviceInLostState = false;
	if (!g_D3DDevice)
	{
		UINT adapterIndex = g_D3DAdapter;
		D3DDEVTYPE devType = g_D3DDevType;

		const int kShaderVersion20 = (1 << 8) + 1;
		DWORD behaviourFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

		// Preserve FPU mode. Benchmarking both in hardware and software vertex processing does not
		// reveal any real differences. If FPU mode is not preserved, bad things will happen, like:
		// * doubles will act like floats
		// * on Firefox/Safari, some JavaScript libraries will stop working (spect.aculo.us, dojo) - case 17513
		// * some random funky FPU exceptions will happen
		HRESULT hr = s_D3D->CreateDevice(adapterIndex, devType, hWnd, behaviourFlags | D3DCREATE_FPU_PRESERVE, &pparams, &g_D3DDevice);
		if (FAILED(hr))
		{
			return false;
		}
	}

	SetD3DDevice((IDirect3DDevice9*)g_D3DDevice, kGfxDeviceEventInitialize);
	SetGfxDevice(new GfxDeviceD3D9());
	InitializeMeshVertexFormatManager();
	GfxBuffer* gfxBuf = GetGfxDevice().CreateVertexBuffer();
	GetGfxDevice().UpdateBuffer(gfxBuf, kGfxBufferModeDynamic, kGfxBufferLabelDefault, 1024, nullptr, 0);

	return true;
}

struct MyVertex {
	float x, y, z;
	unsigned int color;
};

static void SetDefaultGraphicsState()
{
	g_D3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	g_D3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	g_D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_D3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	//g_D3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	//g_D3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	GfxDevice& device = GetGfxDevice();
	GfxDepthState depthState;
	depthState.depthFunc = kFuncLEqual;
	depthState.depthWrite = false;
	device.SetDepthState(device.CreateDepthState(depthState));
}

static void DoRendering(const float* worldMatrix, const float* identityMatrix, float* projectionMatrix, const MyVertex* verts)
{
	// D3D9 case
		// Transformation matrices
	g_D3DDevice->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)worldMatrix);
	g_D3DDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX*)identityMatrix);
	g_D3DDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX*)projectionMatrix);

		// Vertex layout
	g_D3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);

		// Texture stage states to output vertex color
	g_D3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	g_D3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
	g_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	g_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	g_D3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_D3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		// Copy vertex data into our small dynamic vertex buffer. We could have used
		// DrawPrimitiveUP just fine as well.
		void* vbPtr;
		g_D3D9DynamicVB->Lock(0, 0, &vbPtr, D3DLOCK_DISCARD);
		memcpy(vbPtr, verts, sizeof(verts[0]) * 3);
		g_D3D9DynamicVB->Unlock();
		g_D3DDevice->SetStreamSource(0, g_D3D9DynamicVB, 0, sizeof(MyVertex));

		// Draw!
		g_D3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

static DefaultMeshVertexFormat gVertexFormat(VERTEX_FORMAT2(Vertex, Color));

void RenderScene()
{
	g_D3DDevice->BeginScene();
	// A colored triangle. Note that colors will come out differently
	// in D3D9/11 and OpenGL, for example, since they expect color bytes
	// in different ordering.
	MyVertex verts[3] = {
		{ -0.5f, -0.25f, 0, 0xFFff0000 },
		{ 0.5f, -0.25f, 0, 0xFF00ff00 },
		{ 0, 0.5f, 0, 0xFF0000ff },
	};


	// Some transformation matrices: rotate around Z axis for world
	// matrix, identity view matrix, and identity projection matrix.

	float phi = 0;
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);

	float worldMatrix[16] = {
		cosPhi, -sinPhi, 0, 0,
		sinPhi, cosPhi, 0, 0,
		0, 0, 1, 0,
		0, 0, 0.7f, 1,
	};
	float identityMatrix[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	float projectionMatrix[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	// Actual functions defined below
	SetDefaultGraphicsState();
	//DoRendering(worldMatrix, identityMatrix, projectionMatrix, verts);
	//g_D3DDevice->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)worldMatrix);
	//g_D3DDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX*)identityMatrix);
	//g_D3DDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX*)projectionMatrix);

	{
		Matrix4x4f worldMatrix(worldMatrix);
		GetGfxDevice().SetWorldMatrix(worldMatrix);
	}

	{
		Matrix4x4f viewMatrix(identityMatrix);
		GetGfxDevice().SetViewMatrix(viewMatrix);
	}

	{
		Matrix4x4f projectionMatrix(projectionMatrix);
		GetGfxDevice().SetProjectionMatrix(projectionMatrix);
	}

	DynamicVBO& vbo = GetGfxDevice().GetDynamicVBO();
	DynamicVBOChunkHandle meshVBOChunk;
	vbo.GetChunk(sizeof(MyVertex), sizeof(verts), 0, kPrimitiveTriangles, &meshVBOChunk);
	ChannelAssigns* channel = new ChannelAssigns();
	channel->Bind(kShaderChannelVertex, kVertexCompVertex);
	channel->Bind(kShaderChannelColor, kVertexCompColor);
	memcpy(meshVBOChunk.vbPtr, verts, sizeof(verts));
	DynamicVBO::DrawParams params(sizeof(verts), 0, 3, 0, 0);
	vbo.DrawChunk(meshVBOChunk, *channel, gVertexFormat.GetVertexFormat()->GetAvailableChannels(), gVertexFormat.GetVertexFormat()->GetVertexDeclaration(channel->GetSourceMap()), &params);
	vbo.ReleaseChunk(meshVBOChunk, sizeof(verts), 0);
	g_D3DDevice->EndScene();
	g_D3DDevice->Present(NULL, NULL, NULL, NULL);
}


void Shutdown()
{
	if (g_D3DDevice != NULL) g_D3DDevice->Release();
	if (g_D3D != NULL) g_D3D->Release();

	g_D3DDevice = NULL;
	g_D3D = NULL;
}