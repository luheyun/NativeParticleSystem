#include "PluginPrefix.h"
#include "UnityPluginInterface.h"
#include "GfxDevice/d3d/D3D9Includes.h"
#include "GfxDevice/d3d/GfxDeviceD3D9.h"
#include "GfxDevice/GfxDevice.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "GfxDevice/GfxDeviceTypes.h"
#include "GfxDevice/ChannelAssigns.h"


static inline void DebugLog(char* str);
void SetD3DDevice(IDirect3DDevice9* device, GfxDeviceEventType eventType);

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

static int g_DeviceType = -1;

extern "C" void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
	g_DeviceType = -1;

#if SUPPORT_D3D9
	if (deviceType == kGfxRendererD3D9)
	{
		DebugLog("Set D3D9 graphics device\n");
		g_DeviceType = deviceType;
		SetD3DDevice((IDirect3DDevice9*)device, (GfxDeviceEventType)eventType);
		GfxBuffer* gfxBuf = GetGfxDevice().CreateVertexBuffer();
		GetGfxDevice().UpdateBuffer(gfxBuf, kGfxBufferModeDynamic, kGfxBufferLabelDefault, 1024, nullptr, 0);
	}
#endif

	if (deviceType == kGfxRendererOpenGLES20Mobile)
	{
		DebugLog("Set OpenGLES 3.0 graphics device\n");
		g_DeviceType = deviceType;
	}
	else if (deviceType == kGfxRendererOpenGLES30)
	{
		DebugLog("Set OpenGLES 3.0 graphics device\n");
		g_DeviceType = deviceType;
	}

#if SUPPORT_D3D11
	if (deviceType == kGfxRendererD3D11)
	{
		DebugLog("Set D3D11 graphics device, but not supported yet\n");
	}
#endif
}

extern "C" void EXPORT_API UnityRenderEvent(int eventID)
{
	if (eventID == 1)
		DoRender();
}

struct MyVertex {
	float x, y, z;
	unsigned int color;
};

void DoRender()
{
	DebugLog("Do Native Render!");

	// A colored triangle. Note that colors will come out differently
	// in D3D9/11 and OpenGL, for example, since they expect color bytes
	// in different ordering.
	MyVertex verts[3] = {
		{ -0.5f, -0.25f, 0, 0xFFff0000 },
		{ 0.5f, -0.25f, 0, 0xFF00ff00 },
		{ 0, 0.5f, 0, 0xFF0000ff },
	};

	float phi = 0;
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);

	float worldMatrixArray[16] = {
		cosPhi, -sinPhi, 0, 0,
		sinPhi, cosPhi, 0, 0,
		0, 0, 1, 0,
		0, 0, 0.7f, 1,
	};

	float viewMatrixArray[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	float projectionMatrixArray[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	{
		Matrix4x4f worldMatrix(worldMatrixArray);
		GetGfxDevice().SetWorldMatrix(worldMatrix);
	}

	{
		Matrix4x4f viewMatrix(viewMatrixArray);
		GetGfxDevice().SetViewMatrix(viewMatrix);
	}

	{
		Matrix4x4f projectionMatrix(projectionMatrixArray);
		GetGfxDevice().SetProjectionMatrix(projectionMatrix);
	}

	DynamicVBO& vbo = GetGfxDevice().GetDynamicVBO();
	DynamicVBOChunkHandle meshVBOChunk;
	vbo.GetChunk(sizeof(MyVertex), sizeof(verts), 0, kPrimitiveTriangles, &meshVBOChunk); 
	ChannelAssigns* channel = new ChannelAssigns();
	memcpy(meshVBOChunk.vbPtr, verts, sizeof(verts));
	//vbo.DrawChunk(meshVBOChunk, channel);
	vbo.ReleaseChunk(meshVBOChunk, sizeof(verts), 0);
}