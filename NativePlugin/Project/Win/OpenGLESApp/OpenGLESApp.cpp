#include "Include/Precomplie/PluginPrefix.h"
#include <math.h>
#include <tchar.h>   
#include "GfxDevice/GfxDevice.h"
#include "Graphics/Mesh/MeshVertexFormat.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "GfxDevice/ChannelAssigns.h"

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

struct MyVertex {
	float x, y, z;
	unsigned int color;
};

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