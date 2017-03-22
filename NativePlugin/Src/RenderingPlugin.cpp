#include "PluginPrefix.h"
#include "UnityPluginInterface.h"
#include "GfxDevice/d3d/D3D9Includes.h"
#include "GfxDevice/d3d/GfxDeviceD3D9.h"
#include "Graphics/Mesh/VertexData.h"
#include "GfxDevice/VertexDeclaration.h"
#include "GfxDevice/GfxDevice.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "GfxDevice/GfxDeviceTypes.h"
#include "GfxDevice/ChannelAssigns.h"
#include "Graphics/Mesh/MeshVertexFormat.h"
#include "ParticleSystem/ParticleSystem.h"


static inline void DebugLog(char* str);
void SetD3DDevice(IDirect3DDevice9* device, GfxDeviceEventType eventType);

// --------------------------------------------------------------------------
// Allow writing to the Unity debug console from inside DLL land.
extern "C"
{
	void(_stdcall*debugLog)(char*) = nullptr;

	__declspec(dllexport) void StartUp(void(_stdcall*d)(char*))
	{
		debugLog = d;
		DebugLog("Plugin Start Up!");
	}

	ParticleSystem* gPs = nullptr;

	__declspec(dllexport) void CreateParticleSystem()
	{
		// todo
		gPs = new ParticleSystem();
	}

	__declspec(dllexport) void ShutDown()
	{
		debugLog = nullptr;

		if (gPs != nullptr)
			delete gPs;

		gPs = nullptr;
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
LPDIRECT3DDEVICE9 g_D3DDevice = NULL;

extern "C" void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
	g_DeviceType = -1;

#ifdef GFX_SUPPORTS_D3D9
	if (deviceType == kGfxRendererD3D9)
	{
		DebugLog("Set D3D9 graphics device\n");
		g_DeviceType = deviceType;
		g_D3DDevice = (IDirect3DDevice9*)device;
		SetD3DDevice((IDirect3DDevice9*)device, (GfxDeviceEventType)eventType);
		SetGfxDevice(new GfxDeviceD3D9());
		InitializeMeshVertexFormatManager();
		ParticleSystem::Init();
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

#if GFX_SUPPORTS_D3D11
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

static void SetDefaultGraphicsState()
{
	g_D3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	g_D3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	g_D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_D3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	GfxDevice& device = GetGfxDevice();
	GfxDepthState depthState;
	depthState.depthFunc = kFuncLEqual;
	depthState.depthWrite = false;
	device.SetDepthState(device.CreateDepthState(depthState));
}

static DefaultMeshVertexFormat gVertexFormat(VERTEX_FORMAT2(Vertex, Color));

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

	SetDefaultGraphicsState();

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

	g_D3DDevice->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)worldMatrixArray);
	g_D3DDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX*)viewMatrixArray);
	g_D3DDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX*)projectionMatrixArray);

	g_D3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	g_D3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
	g_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	g_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	g_D3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_D3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	/*DynamicVBO& vbo = GetGfxDevice().GetDynamicVBO();
	DynamicVBOChunkHandle meshVBOChunk;
	vbo.GetChunk(sizeof(MyVertex), sizeof(verts), 0, kPrimitiveTriangles, &meshVBOChunk);
	ChannelAssigns* channel = new ChannelAssigns();
	channel->Bind(kShaderChannelVertex, kVertexCompVertex);
	channel->Bind(kShaderChannelColor, kVertexCompColor);
	memcpy(meshVBOChunk.vbPtr, verts, sizeof(verts));
	DynamicVBO::DrawParams params(sizeof(verts), 0, 3, 0, 0);
	vbo.ReleaseChunk(meshVBOChunk, sizeof(verts), 0);
	vbo.DrawChunk(meshVBOChunk, *channel, gVertexFormat.GetVertexFormat()->GetAvailableChannels()
	, gVertexFormat.GetVertexFormat()->GetVertexDeclaration(channel->GetSourceMap()), &params);*/

	ParticleSystem::BeginUpdateAll();
	ParticleSystem::EndUpdateAll();
	ParticleSystem::Prepare();
	ParticleSystem::Render();
}