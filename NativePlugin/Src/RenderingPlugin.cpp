#include "PluginPrefix.h"
#include "Log/Log.h"
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
#include "Mono/NativeUtil.h"
#include "Mono/ScriptingAPI.h"
#include "Input/TimeManager.h"

static void DoRender();
void SetD3DDevice(IDirect3DDevice9* device, GfxDeviceEventType eventType);
static void InitMono();
static void* g_TexturePointer;

class MonoUpdateData
{
public:
	float frameTime;
	float deltaTime;
	Matrix4x4f viewMatrix;
};

void Internal_Update(ScriptingObject* updateData)
{
	MonoUpdateData* pUpdateData = (MonoUpdateData*)GetLogicObjectMemoryLayout(updateData);
	SetFrameTime(pUpdateData->frameTime);
	SetDeltaTime(pUpdateData->deltaTime);
	GetGfxDevice().SetViewMatrix(pUpdateData->viewMatrix);
	ParticleSystem::BeginUpdateAll();
}

static const char* s_RenderingPlugin_IcallNames[] =
{
	//"WNEngine.NativeUtil::Internal_CreateNativeUtil",
	//"WNEngine.NativeUtil::get_EnableLog",
	//"WNEngine.NativeUtil::set_EnableLog",
	"NativePlugin::Internal_Update",
	NULL
};

static const void* s_RenderingPlugin_IcallFuncs[] =
{
	//(const void*)&Internal_CreateNativeUtil_Native,
	//(const void*)&NativeUtil_get_EnableLog,
	//(const void*)&NativeUtil_set_EnableLog,
	(const void*)&Internal_Update,
	NULL
};

void RegisterRenderingPluginBindings()
{
	for (int i = 0; s_RenderingPlugin_IcallNames[i] != NULL; ++i)
	{
		script_add_internal_call(s_RenderingPlugin_IcallNames[i], s_RenderingPlugin_IcallFuncs[i]);
	}
}

static void* LoadPluginExecutable(const char* pluginPath)
{
#if _MSC_VER
	HMODULE hMono = ::GetModuleHandleA(pluginPath);
	if (hMono == NULL)
	{
		hMono = LoadLibraryA(pluginPath);
	}
	return hMono;

#else
	return dlopen(pluginPath, RTLD_NOW);
#endif
}

static void* LoadPluginFunction(void* pluginHandle, const char* name)
{
#if _MSC_VER
	return GetProcAddress((HMODULE)pluginHandle, name);

#else
	return dlsym(pluginHandle, name);
#endif
}

static void UnloadPluginExecutable(void* pluginHandle)
{
#if _MSC_VER
	FreeLibrary((HMODULE)pluginHandle);
#else
	dlclose(pluginHandle);
#endif
}

// --------------------------------------------------------------------------
// Allow writing to the Unity debug console from inside DLL land.
extern "C"
{
	EXPORT_API void StartUp(void(_stdcall*d)(char*))
	{
		SetDebugLog(d);
		DebugLog("Plugin Start Up!");
		InitMonoSystem();
		RegisterRenderingPluginBindings();
		ParticleSystem::Init();
	}

	EXPORT_API void ShutDown()
	{
		SetDebugLog(nullptr);
		ParticleSystem::ShutDown();
	}

	void EXPORT_API SetTextureFromUnity(void* texturePtr)
	{
		g_TexturePointer = texturePtr;
	}

	void EXPORT_API Native_Render()
	{
		DoRender();
	}
}

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

		if (g_D3DDevice != nullptr)
		{
			SetD3DDevice((IDirect3DDevice9*)device, (GfxDeviceEventType)eventType);
			SetGfxDevice(new GfxDeviceD3D9());
			InitializeMeshVertexFormatManager();
			GfxBuffer* gfxBuf = GetGfxDevice().CreateVertexBuffer();
			GetGfxDevice().UpdateBuffer(gfxBuf, kGfxBufferModeDynamic, kGfxBufferLabelDefault, 1024, nullptr, 0);
		}
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
	//g_D3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	//g_D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	//g_D3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	GfxDevice& device = GetGfxDevice();
	GfxDepthState depthState;
	depthState.depthFunc = kFuncLEqual;
	depthState.depthWrite = false;
	device.SetDepthState(device.CreateDepthState(depthState));
}

static DefaultMeshVertexFormat gVertexFormat(VERTEX_FORMAT2(Vertex, Color));
static float phi = 0;

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

	Matrix4x4f worldMatrix(worldMatrixArray);
	GetGfxDevice().SetWorldMatrix(worldMatrix);

	Matrix4x4f viewMatrix(viewMatrixArray);
	GetGfxDevice().SetViewMatrix(viewMatrix);

	Matrix4x4f projectionMatrix(projectionMatrixArray);
	GetGfxDevice().SetProjectionMatrix(projectionMatrix);

	Matrix4x4f mv;
	MultiplyMatrices4x4(&viewMatrix, &worldMatrix, &mv);
	Matrix4x4f mvp;
	MultiplyMatrices4x4(&projectionMatrix, &mv, &mvp);

	g_D3DDevice->SetVertexShaderConstantF(0, mvp.GetPtr(), 4);

	//g_D3DDevice->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)worldMatrixArray);
	//g_D3DDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX*)viewMatrixArray);
	//g_D3DDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX*)projectionMatrixArray);

	//g_D3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	//g_D3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
	//g_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	//g_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	//g_D3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	//g_D3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

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

	//ParticleSystem::BeginUpdateAll();
	ParticleSystem::EndUpdateAll();
	ParticleSystem::Prepare();
	ParticleSystem::Render();
}