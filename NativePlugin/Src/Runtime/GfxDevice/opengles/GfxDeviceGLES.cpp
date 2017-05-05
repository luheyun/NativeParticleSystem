#include "UnityPrefix.h"

#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "GfxDeviceGLES.h"
#include "ApiConstantsGLES.h"
#include "AssertGLES.h"
#include "ContextGLES.h"
#include "DeviceStateGLES.h"
#include "GraphicsCapsGLES.h"
#include "VertexDeclarationGLES.h"
#include "GpuProgramsGLES.h"
#include "TextureIdMapGLES.h"
#include "GlslGpuProgramGLES.h"
#include "GpuProgramParamsGLES.h"
#include "FrameBufferGLES.h"
#include "SparseTextureGLES.h"
#include "TexturesGLES.h"
#include "TextureReadbackGLES.h"
#include "TimerQueryGLES.h"
#include "TransformFeedbackSkinning.h"
#include "GfxContextGLES.h"

#include "Runtime/Math/Matrix4x4.h"
#include "Runtime/Math/Matrix3x3.h"

#include "Runtime/Shaders/GraphicsCaps.h"
#include "Runtime/Shaders/ComputeShader.h"

#include "Runtime/GfxDevice/ChannelAssigns.h"
#include "Runtime/GfxDevice/GfxDeviceObjects.h"

#include "Runtime/Graphics/GraphicsHelper.h"
#include "Runtime/Graphics/Mesh/GenericDynamicVBO.h"
#include "Runtime/Graphics/Texture.h"
#include "Runtime/Camera/RenderManager.h"

#include "Runtime/Shaders/ShaderImpl/ShaderTextureProperty.h"
#include "Runtime/Shaders/ShaderImpl/ShaderImpl.h"
#include "Runtime/Shaders/ShaderImpl/ShaderProgram.h"
#include "Runtime/Shaders/ShaderImpl/ShaderPass.h"
#include "Runtime/Shaders/ShaderImpl/ShaderUtilities.h"

#include "Runtime/Utilities/LogUtility.h"
#include "Runtime/Misc/Plugins.h"

#if UNITY_ANDROID
#include "PlatformDependent/AndroidPlayer/Source/EntryPoint.h"
#include "PlatformDependent/AndroidPlayer/Source/AndroidSystemInfo.h"
#include "PlatformDependent/AndroidPlayer/Source/Android_Profiler.h"
#endif
#if UNITY_EDITOR && (UNITY_WIN || UNITY_LINUX)
#include "WindowGLES.h"
#endif

#define UNITY_DESKTOP (UNITY_WIN || UNITY_LINUX || UNITY_OSX)

#if UNITY_DESKTOP
#	include "Runtime/GfxDevice/opengl/GLContext.h"
#endif

extern GfxDeviceLevelGL g_RequestedGLLevel;
const char* GetGfxDeviceLevelString(GfxDeviceLevelGL deviceLevel);

#if FORCE_DEBUG_BUILD_WEBGL
#	undef UNITY_WEBGL
#	define UNITY_WEBGL 1
#endif//FORCE_DEBUG_BUILD_WEBGL

namespace
{
	void SetDepthState(ApiGLES & api, DeviceStateGLES& state, const DeviceDepthStateGLES* newState);
	void SetBlendState(ApiGLES & api, DeviceStateGLES& state, const DeviceBlendStateGLES* newState);
	void SetRasterState(ApiGLES & api, DeviceStateGLES& state, const DeviceRasterStateGLES* newState);
}//namespace

// A shortcut for places where we cannot include GfxDeviceGLES.h
namespace gles
{
	GfxFramebufferGLES &GetFramebufferGLES() { return ((GfxDeviceGLES &)GetUncheckedRealGfxDevice()).GetFramebuffer(); }
};

GfxDeviceGLES::GfxDeviceGLES() :
	m_Context(NULL)
{
}

GfxDeviceGLES::~GfxDeviceGLES()
{
	ContextGLES::Acquire(); //Acquire context for resource cleanup

#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
	m_DisplayManager.Cleanup ();
#endif

	PluginsSetGraphicsDevice(NULL, m_Renderer, kGfxDeviceEventShutdown);

	// Clear all TF shaders we may have
	TransformFeedbackSkinning::CleanupTransformFeedbackShaders();
	CleanupSharedBuffers();

	m_State.constantBuffers.Clear();
	DeleteDynamicVBO();

	ReleaseBufferManagerGLES();

	vertDeclCache.Clear();

	if (GetGraphicsCaps().gles.hasSamplerObject)
		for(std::size_t i = 0; i < kBuiltinSamplerStateCount; ++i)
			m_Api.DeleteSampler(m_State.builtinSamplers[i]);

	delete m_Context;
	m_Context = NULL;

	ContextGLES::Destroy();
}

void GfxDeviceGLES::OnDeviceCreated(bool callingFromRenderThread)
{
#	if UNITY_DESKTOP
		// needs to activate graphics context on both main & render threads
		ActivateGraphicsContext(GetMainGraphicsContext(), true);
#	endif
}
static const DeviceBlendStateGLES* CreateBlendStateNoColorWriteNoAlphaTest(DeviceStateGLES& state)
{
	GfxBlendState blendState;
	if (g_GraphicsCapsGLES->buggyDisableColorWrite)
	{
		// emulate non-working glColorMask with blending.
		blendState.srcBlend = blendState.srcBlendAlpha = kBlendZero;
		blendState.dstBlend = blendState.dstBlendAlpha = kBlendOne;
	}
	else
	{
		blendState.renderTargetWriteMask = 0;
	}
	return gles::CreateBlendState(state, blendState);
}

static int contextLevelToGLESVersion(GfxDeviceLevelGL in)
{
	switch (in)
	{
	default:
	case kGfxLevelES3:
	case kGfxLevelES31:
	case kGfxLevelES31AEP:
		return 3;
	case kGfxLevelES2:
		return 2;
	}
}

// The content of this function should be in GfxDeviceGLES constructor
// but GfxDeviceGLES instances are created with UNITY_NEW_AS_ROOT which doesn't allow arguments passing.
bool GfxDeviceGLES::Init(GfxDeviceLevelGL deviceLevel)
{
	void* masterContextPointer = NULL;
	g_RequestedGLLevel = deviceLevel;

#	if UNITY_DESKTOP
#		if UNITY_WIN
			SetMasterContextClassName(L"WindowGLClassName");

			GfxDeviceLevelGL CreateMasterGraphicsContext(GfxDeviceLevelGL level);
			deviceLevel = CreateMasterGraphicsContext(deviceLevel);
			if (deviceLevel == kGfxLevelUninitialized)
				return false;
#		endif

		GraphicsContextHandle masterContext = GetMasterGraphicsContext();
		if (!masterContext.IsValid())
			return false;

		SetMainGraphicsContext(masterContext);
		ActivateGraphicsContext(masterContext, true, kGLContextSkipGfxDeviceMakeCurrent);

		GraphicsContextGL* context = OBJECT_FROM_HANDLE(masterContext, GraphicsContextGL);
		masterContextPointer = context;
#	else
		// Read in context version
		ContextGLES::Create(contextLevelToGLESVersion(deviceLevel));
		masterContextPointer = (void*)1;// masterContextPointer should not be null when calling SetActiveContext
#	endif

	GLES_ASSERT(&m_Api, masterContextPointer, "No master context created");

	// Initialize context and states
	g_DeviceStateGLES = &m_State;

	if (IsGfxLevelES2(deviceLevel))
		m_Renderer = kGfxRendererOpenGLES20;
	else if (IsGfxLevelES(deviceLevel))
		m_Renderer = kGfxRendererOpenGLES3x;
	else if (IsGfxLevelCore(deviceLevel))
		m_Renderer = kGfxRendererOpenGLCore;
	else
		GLES_ASSERT(&m_Api, 0, "OPENGL ERROR: Invalid device level");

	m_Context = new GfxContextGLES;

	m_Api.Init(*m_Context, deviceLevel);
	gGL = m_State.api = &m_Api;

	this->SetActiveContext(masterContextPointer);

	m_Api.InitDebug();
	m_Api.debug.Log(Format("OPENGL LOG: GfxDeviceGLES::Init - CreateMasterGraphicsContext\n").c_str());

	printf_console("OPENGL LOG: Creating OpenGL%s%d.%d graphics device ; Context level %s ; Context handle %d\n",
		IsGfxLevelES(deviceLevel) ? " ES " : " ",
		GetGraphicsCaps().gles.majorVersion, GetGraphicsCaps().gles.minorVersion,
		GetGfxDeviceLevelString(deviceLevel),
		gl::GetCurrentContext());

	InitCommonState(m_State);
	InvalidateState();
	m_IsThreadable = true;

	m_GlobalDepthBias = m_GlobalSlopeDepthBias = 0.0f;

	m_AtomicCounterBuffer = NULL;
	for (int i = 0; i < kMaxAtomicCounters; i++)
		m_AtomicCounterSlots[i] = NULL;

	PluginsSetGraphicsDevice(NULL, m_Renderer, kGfxDeviceEventInitialize);

#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
	m_DisplayManager.Initialize ();
#endif
	return true;
}


void GfxDeviceGLES::InitCommonState(DeviceStateGLES& state)
{
	state.depthStateNoDepthAccess = gles::CreateDepthState(state, GfxDepthState(false, kFuncDisabled));
	state.blendStateNoColorNoAlphaTest = CreateBlendStateNoColorWriteNoAlphaTest(state);
	gles::InvalidatePipelineStates(*m_Context, state);

	for (int i = 0; i < kBuiltinSamplerStateCount; i++)
	{
		state.builtinSamplers[i] = 0;
	}

	state.randomWriteTargetMaxIndex = -1;
	for (int i = 0; i < kMaxSupportedRenderTargets; i++)
	{
		state.randomWriteTargetTextures[i] = TextureID();
		state.randomWriteTargetBuffers[i] = ComputeBufferID();
	}

	memset(state.barrierTimes, 0, gl::kBarrierTypeCount * sizeof(BarrierTime));
	state.barrierTimeCounter = 1; // offset start time to 1 to not miss the first needed barrier in any case
	state.requiredBarriers = 0;
}

void GfxDeviceGLES::BeforePluginRender()
{
	// do nothing with gles state: we expect users to reset the state they want themselves
	// but make sure we actually set gles fbo in case it was delayed
	m_Context->GetFramebuffer().Prepare();
	CheckErrorGLES(&m_Api, "OPENGL NATIVE PLUG-IN ERROR", __FILE__, __LINE__); // Check that we haven't generated OpenGL errors before entering user code
}

void GfxDeviceGLES::AfterPluginRender()
{
	CheckErrorGLES(&m_Api, "OPENGL NATIVE PLUG-IN ERROR", __FILE__, __LINE__); // Check that the user plug in code doesn't produce OpenGL errors
	InvalidateState();
}

void GfxDeviceGLES::InvalidateState()
{
	GfxDevice::InvalidateState ();

	// do not touch matrices themselves in transform state, just mark everything as dirty: needed for plugins
	m_State.transformDirtyFlags = TransformState::kWorldViewProjDirty;
	gles::Invalidate(*m_Context, m_State);
	ApplyBackfaceMode();

	m_Context->GetFramebuffer().InvalidateActiveFramebufferState();
}

GfxDeviceLevelGL GfxDeviceGLES::GetDeviceLevel() const
{
	return GetGraphicsCaps().gles.featureLevel;
}

void GfxDeviceGLES::BeginFrame()
{
	DBG_LOG_GLES("BeginFrame()");

	#if UNITY_ANDROID
		Profiler_RenderingStart();
	#endif

	m_InsideFrame = true;

	GfxRenderTargetSetup & def = m_Context->GetFramebuffer().GetDefaultFramebuffer();
	def.color[0]->loadAction = def.colorLoadAction[0] = kGfxRTLoadActionDontCare;
	def.depth->loadAction = def.depthLoadAction = kGfxRTLoadActionDontCare;

	m_Context->GetFramebuffer().Activate(GfxFramebufferGLES::kDefault, true);
}

namespace
{

// Workaround to avoid broken textures with older Adreno drivers.
// Calling glFinish before the first texture upload of a frame seems to fix the problem.
namespace AdrenoTextureUploadWorkaround
{

bool s_FinishCalledThisFrame = false;

void EndFrame()
{
	if (GetGraphicsCaps().gles.buggyTextureUploadSynchronization)
	{
		s_FinishCalledThisFrame = false;
	}
}

void PrepareTextureUpload(GLuint texture)
{
	if (!GetGraphicsCaps().gles.buggyTextureUploadSynchronization)
		return;

	if (s_FinishCalledThisFrame)
		return;

	if (!texture)
		return; // new textures cannot get corrupted

	gGL->Submit(gl::SUBMIT_FINISH);
	s_FinishCalledThisFrame = true;
}

} // namespace AdrenoTextureUploadWorkaround
} // namespace

void GfxDeviceGLES::EndFrame()
{
	// Player guarantees that no RenderTexture is bound at this point.
	m_Context->GetFramebuffer().TryInvalidateDefaultFramebufferDepth();

	// On desktop we do this after present
#	if !UNITY_DESKTOP
		GetBufferManagerGLES()->AdvanceFrame();
#	endif

	AdrenoTextureUploadWorkaround::EndFrame();

	GLES_ASSERT(&m_Api, m_Api.Verify(), "Tracked states don't match with actual OpenGL states");

	m_InsideFrame = false;

	#if UNITY_ANDROID
		Profiler_RenderingEnd();
	#endif

	DBG_LOG_GLES("EndFrame()");
}


void GfxDeviceGLES::RenderTargetBarrier()
{
	if (GetGraphicsCaps().hasBlendAdvanced && !GetGraphicsCaps().hasBlendAdvancedCoherent)
	{
		GLES_CALL(&m_Api, glBlendBarrier);
	}
	
	//TODO: add TextureBarrier/TextureBarrierNV here on desktop (GL_NV_texture_barrier)
}

const DeviceBlendState* GfxDeviceGLES::CreateBlendState(const GfxBlendState& state)
{
	return gles::CreateBlendState(m_State, state);
}
const DeviceDepthState* GfxDeviceGLES::CreateDepthState(const GfxDepthState& state)
{
	GLES_ASSERT(&m_Api, state.depthFunc != kFuncUnknown, "Invalid depth function");
	return gles::CreateDepthState(m_State, state);
}
const DeviceStencilState* GfxDeviceGLES::CreateStencilState(const GfxStencilState& state)
{
	return gles::CreateStencilState(m_State, state);
}
const DeviceRasterState* GfxDeviceGLES::CreateRasterState(const GfxRasterState& state)
{
	return gles::CreateRasterState(m_State, state);
}

void GfxDeviceGLES::SetBlendState(const DeviceBlendState* state)
{
	const DeviceBlendStateGLES* newState = (DeviceBlendStateGLES*)state;

	// disable color writes if we miss color attachment
	if(RenderSurfaceBase_IsDummy(*m_Context->GetFramebuffer().GetPendingFramebuffer().color[0]))
	{
		newState = gles::UpdateColorMask(m_State, newState, 0);
	}
	else if (newState->sourceState.renderTargetWriteMask == 0)
	{
		newState = m_State.blendStateNoColorNoAlphaTest;
	}

	::SetBlendState(m_Api, m_State, newState);
}

void GfxDeviceGLES::SetDepthState(const DeviceDepthState* state)
{
	const DeviceDepthStateGLES* newState = (const DeviceDepthStateGLES*)state;

	// before everything check if we miss depth (to disable depth ops)
	if(gles::IsDummySurface(m_Context->GetFramebuffer().GetPendingFramebuffer().depth))
		newState = m_State.depthStateNoDepthAccess;

	::SetDepthState(m_Api, m_State, newState);
}

void GfxDeviceGLES::SetRasterState(const DeviceRasterState* state)
{
	const DeviceRasterStateGLES* newState = (const DeviceRasterStateGLES*)state;
	if (m_GlobalDepthBias != 0.0f || m_GlobalSlopeDepthBias != 0.0f)
		newState = gles::AddDepthBias(m_State, newState, m_GlobalDepthBias, m_GlobalSlopeDepthBias);
	if (m_ForceCullMode != kCullUnknown)
		newState = gles::AddForceCullMode(m_State, newState, m_ForceCullMode);

	::SetRasterState(m_Api, m_State, newState);
}

void GfxDeviceGLES::SetStencilState(const DeviceStencilState* state, int stencilRef)
{
	const DeviceStencilStateGLES* newState = reinterpret_cast<const DeviceStencilStateGLES*>(state);
	const DeviceStencilStateGLES* curState = m_State.stencilState;
	if(curState == state && stencilRef == m_State.stencilRefValue)
		return;

	m_State.stencilState = newState;
	GLES_ASSERT(&m_Api, m_State.stencilState, "Stentil state object is null");

	m_Api.BindStencilState(&newState->sourceState, stencilRef);

	m_State.stencilRefValue = stencilRef;
}

void GfxDeviceGLES::UpdateSRGBWrite()
{
	if (!GetGraphicsCaps().hasSRGBReadWrite)
		return;

	bool enable = m_State.requestedSRGBWrite;

	// Sometimes sRGB writes happen on linear textures when FRAMEBUFFER_SRGB is set,
	// therefore we explicitly disable FRAMEBUFFER_SRGB for linear textures.
	if (GetGraphicsCaps().buggySRGBWritesOnLinearTextures && m_State.renderTargetsAreLinear > 0)
		enable = false;

	if ((int)enable == m_State.actualSRGBWrite)
		return;

	if (GetGraphicsCaps().gles.hasFramebufferSRGBEnable)
	{
		if (enable)
			this->m_Api.Enable(gl::kFramebufferSRGB);
		else
			this->m_Api.Disable(gl::kFramebufferSRGB);
	}

	m_State.actualSRGBWrite = enable;
}

void GfxDeviceGLES::SetSRGBWrite(bool enable)
{
	m_State.requestedSRGBWrite = enable;
	this->UpdateSRGBWrite();
}

bool GfxDeviceGLES::GetSRGBWrite()
{
	return GetGraphicsCaps().hasSRGBReadWrite && GetGraphicsCaps().gles.hasFramebufferSRGBEnable && m_State.requestedSRGBWrite;
}

void GfxDeviceGLES::ApplyBackfaceMode()
{
	DBG_LOG_GLES("ApplyBackfaceMode");
	if (m_State.appBackfaceMode != m_UserBackfaceMode)
		GLES_CALL(&m_Api, glFrontFace, GL_CCW);
	else
		GLES_CALL(&m_Api, glFrontFace, GL_CW);
}

void GfxDeviceGLES::SetUserBackfaceMode(bool enable)
{
	DBG_LOG_GLES("SetUserBackfaceMode(%s)", GetBoolString(enable));
	if (m_UserBackfaceMode == enable)
		return;

	m_UserBackfaceMode = enable;
	ApplyBackfaceMode();
}

void GfxDeviceGLES::SetForceCullMode (CullMode mode)
{
	DBG_LOG_GLES("SetForceCullMode(%i)", mode);
	if (m_ForceCullMode == mode)
		return;
	m_ForceCullMode = mode;
	
	// reapply raster state (will take changed m_ForceCullMode into account)
	SetRasterState(m_State.rasterState);
}

void GfxDeviceGLES::SetWireframe(bool wire)
{
#	if UNITY_EDITOR
		m_Api.SetPolygonMode(wire);
#	endif//UNITY_EDITOR
}

bool GfxDeviceGLES::GetWireframe() const
{
#	if UNITY_EDITOR
		return m_Api.IsEnabled(gl::kPolygonOffsetLine);
#	else
		return false;
#	endif
}

void GfxDeviceGLES::SetInvertProjectionMatrix(bool enable)
{
	DebugAssert(!enable); // projection should never be flipped upside down on OpenGL
}

void GfxDeviceGLES::SetProjectionMatrix(const Matrix4x4f& matrix)
{
	DBG_LOG_GLES("SetProjectionMatrix(...)");
	GfxDevice::SetProjectionMatrix(matrix);

	m_State.transformDirtyFlags |= TransformState::kProjDirty;
}

void GfxDeviceGLES::SetWorldMatrix(const Matrix4x4f& matrix)
{
	GfxDevice::SetWorldMatrix(matrix);

	m_State.transformDirtyFlags |= TransformState::kWorldDirty;
}

void GfxDeviceGLES::SetViewMatrix(const Matrix4x4f& matrix)
{
	GfxDevice::SetViewMatrix(matrix);

	m_State.transformDirtyFlags |= TransformState::kWorldViewDirty;
}

void GfxDeviceGLES::SetBackfaceMode(bool backface)
{
	DBG_LOG_GLES("SetBackfaceMode(%s)", backface ? "back" : "front");
	if (m_State.appBackfaceMode != backface)
	{
		m_State.appBackfaceMode = backface;
		ApplyBackfaceMode();
	}
}

void GfxDeviceGLES::SetViewport(const RectInt& rect)
{
	DBG_LOG_GLES("SetViewport(%d, %d, %d, %d)", rect.x, rect.y, rect.width, rect.height);

	m_State.viewport = rect;
	m_Context->GetFramebuffer().SetViewport(rect);
}

RectInt GfxDeviceGLES::GetViewport() const
{
	DBG_LOG_GLES("GetViewport()");
	return m_State.viewport;
}

void GfxDeviceGLES::DisableScissor()
{
	DBG_LOG_GLES("DisableScissor()");
	if(m_State.scissor)
	{
		m_Api.Disable(gl::kScissorTest);
		m_State.scissor = false;
	}
}

bool GfxDeviceGLES::IsScissorEnabled() const
{
	DBG_LOG_GLES("IsScissorEnabled():returns %s", m_State.scissor == 1 ? "True" : "False");
	return m_State.scissor;
}

void GfxDeviceGLES::SetScissorRect(const RectInt& rect)
{
	DBG_LOG_GLES("SetScissorRect(%d, %d, %d, %d)", rect.x, rect.y, rect.width, rect.height);

	if(!m_State.scissor)
	{
		m_Api.Enable(gl::kScissorTest);
		m_State.scissor = true;
	}

	m_State.scissorRect = rect;
	m_Context->GetFramebuffer().SetScissor(rect);
}

RectInt GfxDeviceGLES::GetScissorRect() const
{
	DBG_LOG_GLES("GetScissorRect()");
	return m_State.scissorRect;
}


void GfxDeviceGLES::SetTextures(ShaderType shaderType, int count, const GfxTextureParam* textures)
{
	for (int i = 0; i < count; ++i, ++textures)
	{
		const int unit = textures->textureUnit;
		const TextureID texture = textures->texture;

		GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(texture);
		if (texInfo)
		{
			MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureFetch);
			const TextureDimension dim = texInfo->dim;
			gles::SetTexture(m_State, texInfo->texture, dim, unit);
		}
		else
		{
			gles::SetTexture(m_State, 0, kTexDim2D, unit);
		}
	}
}

void GfxDeviceGLES::SetTextureParams(TextureID texture, TextureDimension texDim, TextureFilterMode filter, TextureWrapMode wrap, int anisoLevel, float mipBias, bool hasMipMap, TextureColorSpace colorSpace, ShadowSamplingMode shadowSamplingMode)
{
	DBG_LOG_GLES("SetTextureParams()");
	
	if (texDim < kTexDimFirst || texDim > kTexDimLast)
	{
		// If the texture's dimensions have not been initialized yet, bail out.
		// This might happen because if, for example, a texture has been created from code and it has yet
		// to be initialized.
		return;
	}
		
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrCreate(&m_Api, texDim, texture);

	gles::SetTexture(m_State, texInfo->texture, texDim, 0);

	if (!hasMipMap && filter == kTexFilterTrilinear)
		filter = kTexFilterBilinear;

	m_Api.TextureSampler(texInfo->texture, texDim, filter, wrap, anisoLevel, mipBias, hasMipMap, shadowSamplingMode);
}

void GfxDeviceGLES::SetShadersThreadable(GpuProgram* programs[kShaderTypeCount], const GpuProgramParameters* params[kShaderTypeCount], UInt8 const * const paramsBuffer[kShaderTypeCount])
{
	GpuProgram* vertexProgram = programs[kShaderVertex];

	// GLSL is only supported like this:
	// vertex shader actually is both vertex & fragment linked program
	// fragment shader is unused

	if (vertexProgram && vertexProgram->GetImplType() == kShaderImplBoth)
	{
		m_State.activeProgram = vertexProgram;
		m_State.activeProgramParams = params[kShaderVertex];
	}
	else
	{
		GLES_ASSERT(&m_Api, vertexProgram == 0, "Our implementation reauires to use kShaderImplBoth for shaders");
		m_State.activeProgram = 0;
		m_State.activeProgramParams = 0;
	}

	if (m_State.activeProgram)
	{
		GlslGpuProgramGLES& prog = static_cast<GlslGpuProgramGLES&>(*m_State.activeProgram);
		prog.ApplyGpuProgramGLES(*m_State.activeProgramParams, paramsBuffer[kShaderVertex]);
		m_State.activeConstantBuffers = &m_State.activeProgramParams->GetConstantBuffers();
		m_BuiltinParamIndices[kShaderVertex] = &m_State.activeProgramParams->GetBuiltinParams();
		m_State.activeUniformCache = &prog.m_UniformCache;
	}
}


bool GfxDeviceGLES::IsShaderActive(ShaderType type) const
{
	DBG_LOG_GLES("IsShaderActive(%s):", GetShaderTypeString(type));

	if (!m_State.activeProgram)
		return false;

	GlslGpuProgramGLES *prog = (GlslGpuProgramGLES *)m_State.activeProgram;
	return prog->HasShaderStage(type);

}

void GfxDeviceGLES::DestroySubProgram(ShaderLab::SubProgram* subprogram)
{
	//@TODO
	//if (m_State.activeProgram == program)
	//{
	//  m_State.activeProgram = NULL;
	//  m_State.activeProgramProps = NULL;
	//}
	delete subprogram;
}

void GfxDeviceGLES::DestroyGpuProgram(GpuProgram const * const program)
{
	GLES_ASSERT(&m_Api, program, "We can only delete a none null program");
	delete program;

	for (int pt = 0; pt < kShaderTypeCount; ++pt)
	{
		if (m_State.activeProgram == program)
		{
			m_State.activeProgram = NULL;
			m_State.activeProgramParams = NULL;
		}
	}
}


DynamicVBO* GfxDeviceGLES::CreateDynamicVBO()
{
	if (GetGraphicsCaps().gles.hasCircularBuffer)
	{
		// TODO Different settings for mobiles? Now we start with 2 x 1MB Vertex buffers.
		const UInt32 kInitialVBSize = 1 * 1024 * 1024;
		const UInt32 kInitialIBSize = 64 * 1024;
		return new GenericDynamicVBO(*this, kGfxBufferModeCircular, kInitialVBSize, kInitialIBSize);
	}
	else
		return new GenericDynamicVBO(*this);
}


//
// render surface handling
//

size_t GfxDeviceGLES::RenderSurfaceStructMemorySize(bool /*colorSurface*/)
{
	return sizeof(RenderSurfaceGLES);
}

void GfxDeviceGLES::AliasRenderSurfacePlatform(RenderSurfaceBase* rs, TextureID origTexID)
{
	gles::AliasRenderSurface((RenderSurfaceGLES*)rs, origTexID);
}

bool GfxDeviceGLES::CreateColorRenderSurfacePlatform(RenderSurfaceBase* rs, RenderTextureFormat format)
{
	const TextureColorSpace colorSpace = rs->flags & kSurfaceCreateSRGB ? kTexColorSpaceSRGB : kTexColorSpaceLinear;
	gles::CreateColorRenderSurface(&m_Api, (RenderSurfaceGLES*)rs, m_Api.translate.GetFormat(format, colorSpace));
	return true;
}
bool GfxDeviceGLES::CreateDepthRenderSurfacePlatform(RenderSurfaceBase* rs, DepthBufferFormat format)
{
	gles::CreateDepthRenderSurface(&m_Api, (RenderSurfaceGLES*)rs, m_Api.translate.GetFormat(format));
	return true;
}
void GfxDeviceGLES::DestroyRenderSurfacePlatform(RenderSurfaceBase* rs)
{
	m_Context->GetFramebuffer().ReleaseFramebuffer(rs, m_Context);
}

// This will copy whole platform render surface descriptor (and not just store argument pointer)
void GfxDeviceGLES::SetBackBufferColorDepthSurface(RenderSurfaceBase* color, RenderSurfaceBase* depth)
{
	GetFramebuffer().SetBackBufferColorDepthSurface(color, depth);
	GfxDevice::SetBackBufferColorDepthSurface(color, depth);
}

void GfxDeviceGLES::SetRenderTargets(const GfxRenderTargetSetup& rt)
{
	GfxFramebufferGLES& framebuffer = m_Context->GetFramebuffer();

	GLESRenderTargetSetup newRT(rt);
	GLESRenderTargetSetup oldRT(framebuffer.GetPendingFramebuffer());

	if(newRT == oldRT && !(rt.flags & kFlagForceSetRT))
		return;

	GLES_ASSERT(&m_Api, GetGraphicsCaps().hasRenderToTexture, "Render to texture is not supported");
	GetRealGfxDevice().GetFrameStats().AddRenderTextureChange();

	framebuffer.Activate(rt);
	if (GetGraphicsCaps().gles.requirePrepareFramebuffer)
		framebuffer.Prepare();

	if (GetGraphicsCaps().buggySRGBWritesOnLinearTextures)
	{
		bool allLinear = true;
		for (int i = 0; i < rt.colorCount; i++)
			allLinear &= (rt.color[i]->flags & kSurfaceCreateSRGB) == 0;
		bool isBackBuffer = rt.color[0]->backBuffer;
		m_State.renderTargetsAreLinear = allLinear && !isBackBuffer;
		this->UpdateSRGBWrite();
	}
}

void GfxDeviceGLES::DiscardContents(RenderSurfaceHandle& rs)
{
	if(rs.IsValid())
		m_Context->GetFramebuffer().DiscardContents(rs);
}

RenderSurfaceHandle GfxDeviceGLES::GetActiveRenderColorSurface(int index) const
{
	GLES_ASSERT(&m_Api, 0 <= index && index <= kMaxSupportedRenderTargets, "Too many render color surface used");

	return RenderSurfaceHandle(m_Context->GetFramebuffer().GetPendingFramebuffer().color[index]);
}

RenderSurfaceHandle GfxDeviceGLES::GetActiveRenderDepthSurface() const
{
	return RenderSurfaceHandle(m_Context->GetFramebuffer().GetPendingFramebuffer().depth);
}

int GfxDeviceGLES::GetCurrentTargetAA() const
{
	return m_Context->GetFramebuffer().GetSamplesCount();
}

int GfxDeviceGLES::GetActiveRenderTargetCount () const
{
	return m_Context->GetFramebuffer().GetPendingFramebuffer().colorCount;
}

GfxFramebufferGLES& GfxDeviceGLES::GetFramebuffer()
{
	return m_Context->GetFramebuffer();
}

void GfxDeviceGLES::ResolveColorSurface(RenderSurfaceHandle srcHandle, RenderSurfaceHandle dstHandle)
{
	GLES_ASSERT(&m_Api, srcHandle.IsValid() && dstHandle.IsValid(), "Invalid RenderSurface");
	
	RenderSurfaceGLES *src = (RenderSurfaceGLES *)srcHandle.object, *dst = (RenderSurfaceGLES *)dstHandle.object;

	if(src->colorSurface == false || dst->colorSurface == false)
	{
		WarningString("RenderTexture: Resolving non-color surfaces.");
		return;
	}

	GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(dst->textureID);
	GLuint srcBuf = (GLuint)((RenderSurfaceGLES*)src)->buffer;
	if (srcBuf == 0 || !texInfo || texInfo->texture == 0)
	{
		WarningString("RenderTexture: Resolving NULL buffers.");
		return;
	}
	MemoryBarrierImmediate(texInfo->imageWriteTime, gl::kBarrierFramebuffer);
	m_Context->GetFramebuffer().Prepare();
	m_Context->GetFramebuffer().ReadbackResolveMSAA(dst, src);
}

void GfxDeviceGLES::ResolveDepthIntoTexture(RenderSurfaceHandle /*colorHandle*/, RenderSurfaceHandle depthHandle)
{
	GfxFramebufferGLES& framebuffer = m_Context->GetFramebuffer();

	RenderSurfaceGLES* dst = (RenderSurfaceGLES *)depthHandle.object;
	GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(dst->textureID);
	if (texInfo)
		MemoryBarrierImmediate(texInfo->imageWriteTime, gl::kBarrierFramebuffer);
	
	framebuffer.Prepare();
	framebuffer.ReadbackDepthIntoRenderTexture(dst);
}

bool GfxDeviceGLES::CaptureScreenshot( int left, int bottom, int width, int height, UInt8* rgba32 )
{
	m_Context->GetFramebuffer().Prepare();
	m_Api.BindFramebuffer(gl::kReadFramebuffer, m_Api.GetFramebufferBinding(gl::kDrawFramebuffer));
	GLES_CALL(&m_Api, glReadPixels,  left, bottom, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba32);
	return true;
}

bool GfxDeviceGLES::ReadbackImage(ImageReference& image, int left, int bottom, int width, int height, int destX, int destY)
{
	GfxFramebufferGLES& framebuffer = m_Context->GetFramebuffer();
	
	framebuffer.Prepare();
	return framebuffer.ReadbackImage(image, left, bottom, width, height, destX, destY);
}

void GfxDeviceGLES::GrabIntoRenderTexture(RenderSurfaceHandle rs, RenderSurfaceHandle rd, int x, int y, int width, int height)
{
	if(!rs.IsValid() || rs.object->backBuffer)
		return;

	GfxFramebufferGLES& framebuffer = m_Context->GetFramebuffer();

	framebuffer.Prepare();
	framebuffer.GrabIntoRenderTexture((RenderSurfaceGLES *)rs.object, x, y, width, height); //memory barriers are handled in this func
}

void GfxDeviceGLES::SetActiveContext(void* context)
{
#if UNITY_WIN || UNITY_LINUX || UNITY_OSX
	if (GraphicsContextGL* glctx = static_cast<GraphicsContextGL*>(context))
	{
		context = glctx->GetContext();
		if (context != (void*)gl::GetCurrentContext())
		{
			ActivateGraphicsContextGL (*glctx, kGLContextSkipInvalidateState | kGLContextSkipUnbindObjects | kGLContextSkipFlush);
		}
	}
#endif

	AssertFormatMsg(context == (void*)1 || (void*)gl::GetCurrentContext() == context, "The context (%p) must already be the active context (%p)", context, (void*)gl::GetCurrentContext());
	GLES_ASSERT(&m_Api, context, "'context' must be a pointer to a valid OpenGL context");

	m_Context->MakeCurrent(m_Api, context);

	// We also invalidate the API, because we need to re-bind all objects
	// so that their state reflects possible changes in other contexts.
	gles::Invalidate(*m_Context, m_State);

	ProcessPendingMipGens();

	m_Context->GetFramebuffer().ActiveContextChanged(&m_BackBufferColor.object, &m_BackBufferDepth.object);
}

void GfxDeviceGLES::PresentFrame()
{
	DBG_LOG_GLES("====================================");
	DBG_LOG_GLES("PresentFrame");
	DBG_LOG_GLES("====================================");

	if (GetGraphicsCaps().gles.requireClearAlpha)
	{
		// Need to make sure we clear to the default framebuffer, in case it has been set by custom RenderTexture drawing code (case 719438)
		m_Context->GetFramebuffer().MakeCurrentFramebuffer(GfxFramebufferGLES::kDefault);

		// Work around for blending issues (must disable while video plays)
		m_Api.Clear(GL_COLOR_BUFFER_BIT, ColorRGBAf(0, 0, 0, 1), true);
	}

#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
	m_DisplayManager.Present (*this);
#endif

	ContextGLES::Present();
}

// Acquire thread ownership on the calling thread. Worker releases ownership.
void GfxDeviceGLES::AcquireThreadOwnership()
{
	ContextGLES::Acquire();
}

// Release thread ownership on the calling thread. Worker acquires ownership.
void GfxDeviceGLES::ReleaseThreadOwnership()
{
	// Flush to make sure that commands enter gpu in correct order.
	// For example glTexSubImage can cause issues without flushing.
	m_Api.Submit();
	
	ContextGLES::Release();
}

// ---------- uploading textures

void GfxDeviceGLES::UploadTexture2D(TextureID texture, TextureDimension dimension, const UInt8* srcData, int srcSize, int width, int height, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureUsageMode usageMode, TextureColorSpace colorSpace)
{
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrCreate(&m_Api, dimension, texture);
	AdrenoTextureUploadWorkaround::PrepareTextureUpload(texInfo->texture);
	MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureUpdate);
	int uploadedSize = gles::UploadTexture(&m_Api, *texInfo, format, srcData, 0, width, height, 1, mipCount, uploadFlags, colorSpace, usageMode);
	
	REGISTER_EXTERNAL_GFX_DEALLOCATION((intptr_t)texture.m_ID);
	REGISTER_EXTERNAL_GFX_ALLOCATION_REF((intptr_t)texture.m_ID, uploadedSize, texture.m_ID);
}

void GfxDeviceGLES::UploadTextureSubData2D(TextureID texture, const UInt8* srcData, int srcSize, int mipLevel, int x, int y, int width, int height, TextureFormat format, TextureColorSpace colorSpace)
{
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrCreate(&m_Api, kTexDim2D, texture);
	AdrenoTextureUploadWorkaround::PrepareTextureUpload(texInfo->texture);
	MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureUpdate);
	gles::UploadTexture2DSubData(&m_Api, texInfo->texture, format, srcData, mipLevel, x, y, width, height, colorSpace);
}

void GfxDeviceGLES::UploadTextureCube(TextureID texture, const UInt8* srcData, int srcSize, int faceDataSize, int size, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureColorSpace colorSpace)
{
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrCreate(&m_Api, kTexDimCUBE, texture);
	AdrenoTextureUploadWorkaround::PrepareTextureUpload(texInfo->texture);
	MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureUpdate);
	int uploadedSize = gles::UploadTexture(&m_Api, *texInfo, format, srcData, faceDataSize, size, size, 1, mipCount, uploadFlags, colorSpace, kTexUsageNone);
	
	REGISTER_EXTERNAL_GFX_DEALLOCATION((intptr_t)texture.m_ID);
	REGISTER_EXTERNAL_GFX_ALLOCATION_REF((intptr_t)texture.m_ID, uploadedSize, texture.m_ID);
}

void GfxDeviceGLES::UploadTexture3D(TextureID texture, const UInt8* srcData, int srcSize, int width, int height, int depth, TextureFormat format, int mipCount, UInt32 uploadFlags)
{
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrCreate(&m_Api, kTexDim3D, texture);
	AdrenoTextureUploadWorkaround::PrepareTextureUpload(texInfo->texture);
	MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureUpdate);
	int uploadedSize = gles::UploadTexture(&m_Api, *texInfo, format, srcData, 0, width, height, depth, mipCount, uploadFlags, kTexColorSpaceLinear, kTexUsageNone);
	
	REGISTER_EXTERNAL_GFX_DEALLOCATION((intptr_t)texture.m_ID);
	REGISTER_EXTERNAL_GFX_ALLOCATION_REF((intptr_t)texture.m_ID, uploadedSize, texture.m_ID);
}

void GfxDeviceGLES::UploadTexture2DArray(TextureID texture, const UInt8* srcData, size_t elementSize, int width, int height, int depth, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureColorSpace colorSpace)
{
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrCreate(&m_Api, kTexDim2DArray, texture);
	AdrenoTextureUploadWorkaround::PrepareTextureUpload(texInfo->texture);
	MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureUpdate);
	int uploadedSize = gles::UploadTexture(&m_Api, *texInfo, format, srcData, elementSize, width, height, depth, mipCount, uploadFlags, colorSpace, kTexUsageNone);

	REGISTER_EXTERNAL_GFX_DEALLOCATION(texture.m_ID);
	REGISTER_EXTERNAL_GFX_ALLOCATION_REF(texture.m_ID, uploadedSize, texture.m_ID);
}


void GfxDeviceGLES::DeleteTexture(TextureID texture)
{
	GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(texture);
	if (texInfo == NULL)
		return;

	DeleteSparseTextureGLES(texture); // delete sparse texture data if needed

	REGISTER_EXTERNAL_GFX_DEALLOCATION((intptr_t)texture.m_ID);

	// invalidate texture unit states that used this texture
	for (int i = 0; i < GetGraphicsCaps().maxTexUnits; ++i)
	{
		TextureUnitStateGLES& currTexture = m_State.textures[i];
		if (currTexture.texID == texInfo->texture)
			gles::Invalidate(currTexture);
	}

	TextureIdMap::RemoveTexture(texture);

	m_Api.DeleteTexture(texInfo->texture); // DeleteTexture sets the param to 0 which enforce to call it last deterministically
	UNITY_DELETE(texInfo, kMemGfxDevice);
}

SparseTextureInfo GfxDeviceGLES::CreateSparseTexture(TextureID texture, int width, int height, TextureFormat textureFormat, int mipCount, TextureColorSpace colorSpace)
{
	const gl::TexFormat format = gGL->translate.GetFormat(textureFormat, colorSpace);
	SparseTextureInfo info = ::CreateSparseTextureGLES(texture, width, height, format, mipCount);
	return info;
}

void GfxDeviceGLES::UploadSparseTextureTile(TextureID texture, int tileX, int tileY, int mip, const UInt8* srcData, int srcSize, int srcPitch)
{
	::UploadSparseTextureTileGLES(texture, tileX, tileY, mip, srcData, srcSize, srcPitch);
}

void GfxDeviceGLES::UpdateConstantBuffer(int id, size_t size, const void* data)
{
	m_State.constantBuffers.UpdateCB(id, size, data);
}


static bool CheckCopyTextureArguments(const ApiGLES& api, GLESTexture* srcTexture, GLESTexture* dstTexture)
{
	if (!srcTexture || !srcTexture->texture)
	{
		ErrorStringMsg("Graphics.CopyTexture could not find source GL texture object. Maybe it is a RenderTexture that is not created yet?");
		return false;
	}
	if (!dstTexture || !dstTexture->texture)
	{
		ErrorStringMsg("Graphics.CopyTexture could not find destination GL texture object. Maybe it is a RenderTexture that is not created yet?");
		return false;
	}

	const FormatDescGLES& srcFormatDesc = api.translate.GetFormatDesc(srcTexture->format);
	const FormatDescGLES& dstFormatDesc = api.translate.GetFormatDesc(dstTexture->format);
	if (srcFormatDesc.blockSize != dstFormatDesc.blockSize)
	{
		ErrorStringMsg(
			"Graphics.CopyTexture can only copy between same texture format groups (OpenGL internal formats: src=%i, blockSize=%i ; dst=%i, blockSize=%i)",
			srcFormatDesc.internalFormat, srcFormatDesc.blockSize, dstFormatDesc.internalFormat, dstFormatDesc.blockSize);
		return false;
	}

	return true;
}

void GfxDeviceGLES::CopyTexture(TextureID src, TextureID dst)
{
	GLESTexture* srcTexture = (GLESTexture*)TextureIdMap::QueryNativeTexture(src);
	GLESTexture* dstTexture = (GLESTexture*)TextureIdMap::QueryNativeTexture(dst);
	if (!CheckCopyTextureArguments(m_Api, srcTexture, dstTexture))
		return;

	// just asserts here; actual error checking should be done in higher level code
	AssertFormatMsg(srcTexture->dim == dstTexture->dim, "OPENGL: Graphics.CopyTexture called on textures of different type (%i vs %i)?", srcTexture->dim, dstTexture->dim);
	AssertFormatMsg(srcTexture->mipCount == dstTexture->mipCount, "OPENGL: Graphics.CopyTexture called on textures with different mip count (%i vs %i)?", srcTexture->mipCount, dstTexture->mipCount);
	AssertMsg(srcTexture->width == dstTexture->width && srcTexture->height == dstTexture->height && srcTexture->layers == dstTexture->layers, "OPENGL: Graphics.CopyTexture called on textures of different sizes?");

	const bool layersIsZ = dstTexture->dim == kTexDim3D;

	m_Api.CopyTextureImage(
		srcTexture->texture, srcTexture->dim, srcTexture->format, 0, 0, 0, 0, 0,
		dstTexture->texture, dstTexture->dim, dstTexture->format, 0, 0, 0, 0, 0,
		layersIsZ ? 1 : srcTexture->layers, srcTexture->mipCount,
		srcTexture->width, srcTexture->height, layersIsZ ? srcTexture->layers : 1);
}

void GfxDeviceGLES::CopyTexture(TextureID src, int srcElement, int srcMip, int srcMipCount, TextureID dst, int dstElement, int dstMip, int dstMipCount)
{
	GLESTexture* srcTexture = (GLESTexture*)TextureIdMap::QueryNativeTexture(src);
	GLESTexture* dstTexture = (GLESTexture*)TextureIdMap::QueryNativeTexture(dst);
	if (!CheckCopyTextureArguments(m_Api, srcTexture, dstTexture))
		return;

	AssertMsg(
		std::max<GLint>(srcTexture->width >> srcMip, 1) == std::max<GLint>(dstTexture->width >> dstMip, 1) &&
		std::max<GLint>(srcTexture->height >> srcMip, 1) == std::max<GLint>(dstTexture->height >> dstMip, 1),
		"OPENGL: Graphics.CopyTexture called on textures of different sizes?");

	m_Api.CopyTextureImage(
		srcTexture->texture, srcTexture->dim, srcTexture->format, srcElement, srcMip, 0, 0, 0,
		dstTexture->texture, dstTexture->dim, dstTexture->format, dstElement, dstMip, 0, 0, 0,
		1, 1,
		std::max<GLint>(srcTexture->width >> srcMip, 1), std::max<GLint>(srcTexture->height >> srcMip, 1), 1);
}

void GfxDeviceGLES::CopyTexture(TextureID src, int srcElement, int srcMip, int srcMipCount, int srcX, int srcY, int srcWidth, int srcHeight, TextureID dst, int dstElement, int dstMip, int dstMipCount, int dstX, int dstY)
{
	GLESTexture* srcTexture = (GLESTexture*)TextureIdMap::QueryNativeTexture(src);
	GLESTexture* dstTexture = (GLESTexture*)TextureIdMap::QueryNativeTexture(dst);
	if (!CheckCopyTextureArguments(m_Api, srcTexture, dstTexture))
		return;

	AssertMsg(srcTexture->dim != kTexDim3D && dstTexture->dim != kTexDim3D, "OPENGL: Graphics.CopyTexture called on 3D textures which is not supported");

	m_Api.CopyTextureImage(
		srcTexture->texture, srcTexture->dim, srcTexture->format, srcElement, srcMip, srcX, srcY, 0,
		dstTexture->texture, dstTexture->dim, dstTexture->format, dstElement, dstMip, dstX, dstY, 0,
		1, 1, srcWidth, srcHeight, 1);
}

// -- skinning --

GPUSkinPoseBuffer* GfxDeviceGLES::CreateGPUSkinPoseBuffer()
{
	return GetGraphicsCaps().hasGPUSkinning
		? UNITY_NEW(TransformFeedbackSkinPoseBuffer, kMemGfxDevice)
		: NULL;
}

void GfxDeviceGLES::DeleteGPUSkinPoseBuffer(GPUSkinPoseBuffer* poseBuffer)
{
	TransformFeedbackSkinPoseBuffer* realBuffer = static_cast<TransformFeedbackSkinPoseBuffer*>(poseBuffer);
	UNITY_DELETE(realBuffer, kMemGfxDevice);
}

void GfxDeviceGLES::UpdateSkinPoseBuffer(GPUSkinPoseBuffer* poseBuffer, const Matrix4x4f* boneMatrices, int boneCount)
{
	static_cast<TransformFeedbackSkinPoseBuffer*>(poseBuffer)->Update(boneMatrices, boneCount);
}

void GfxDeviceGLES::SkinOnGPU(const VertexStreamSource& sourceMesh,
	GfxBuffer* skinBuffer,
	GPUSkinPoseBuffer* poseBuffer,
	GfxBuffer* destBuffer,
	int vertexCount,
	int bonesPerVertex,
	UInt32 channelMask,
	bool lastThisFrame)
{
	TransformFeedbackSkinning::SkinMesh(sourceMesh,
		skinBuffer,
		poseBuffer,
		destBuffer,
		vertexCount,
		bonesPerVertex,
		channelMask,
		lastThisFrame);
}

// -- context --

GfxDevice::PresentMode GfxDeviceGLES::GetPresentMode()
{
#if UNITY_APPLE_PVR || UNITY_WEBGL
	return kPresentAfterDraw;
#else
	return kPresentBeforeUpdate;
#endif
}

bool GfxDeviceGLES::IsValidState()
{
	return ContextGLES::IsValid();
}

bool GfxDeviceGLES::HandleInvalidState()
{
	bool needReload;
	if (!ContextGLES::HandleInvalidState(&needReload))
		return false;

	if (needReload)
		ReloadResources();
	InvalidateState();
	m_Context->GetFramebuffer().UpdateDefaultFramebufferViewport();

	return true;
}

void GfxDeviceGLES::Flush()
{
	m_Api.Submit(gl::SUBMIT_FLUSH);
}

void GfxDeviceGLES::FinishRendering()
{
	m_Api.Submit(gl::SUBMIT_FINISH);
}

bool GfxDeviceGLES::ActivateDisplay(const UInt32 displayId)
{
#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
	return m_DisplayManager.Activate (displayId);
#else
	return false;
#endif
}

bool GfxDeviceGLES::SetDisplayTarget(const UInt32 displayId)
{
#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
	return m_DisplayManager.Setup (*this, displayId);
#else
	return false;
#endif
}

// -- Timer queries --

#if ENABLE_PROFILER

void GfxDeviceGLES::BeginProfileEvent(const char* name)
{
	m_Api.DebugPushMarker(name);
}
void GfxDeviceGLES::EndProfileEvent()
{
	m_Api.DebugPopMarker();
}

GfxTimerQuery* GfxDeviceGLES::CreateTimerQuery()
{
	if (GetGraphicsCaps().hasTimerQuery)
		return UNITY_NEW(TimerQueryGLES, kMemGfxDevice);
	return NULL;
}
void GfxDeviceGLES::DeleteTimerQuery(GfxTimerQuery* query)
{
	UNITY_DELETE(query, kMemGfxDevice);
}
void GfxDeviceGLES::BeginTimerQueries()
{
	if (!GetGraphicsCaps().hasTimerQuery)
		return;

	g_TimerQueriesGLES.BeginTimerQueries();
}
void GfxDeviceGLES::EndTimerQueries()
{
	if (!GetGraphicsCaps().hasTimerQuery)
		return;

	g_TimerQueriesGLES.EndTimerQueries();
}

#endif // ENABLE_PROFILER

void GfxDeviceGLES::SetTextureName(TextureID tex, const char* name)
{
	m_Api.DebugLabel(gl::kTexture, ((GLESTexture*)TextureIdMap::QueryNativeTexture(tex))->texture, 0, name);
}

void GfxDeviceGLES::SetRenderSurfaceName(RenderSurfaceBase* rs_, const char* name)
{
	RenderSurfaceGLES* rs = (RenderSurfaceGLES*)rs_;
	if(g_GraphicsCapsGLES->hasDebugLabel && !gles::IsDummySurface(rs))
	{
		if(rs->textureID.m_ID)
			m_Api.DebugLabel(gl::kTexture, ((GLESTexture*)TextureIdMap::QueryNativeTexture(rs->textureID))->texture, 0, name);
		else
			m_Api.DebugLabel(gl::kRenderbuffer, rs->buffer, 0, name);
	}
}

void GfxDeviceGLES::SetBufferName(GfxBuffer* buffer, const char* name)
{
	const GLuint bufferName = buffer->GetTarget() == kGfxBufferTargetIndex ? static_cast<IndexBufferGLES*>(buffer)->GetGLName() : static_cast<VertexBufferGLES*>(buffer)->GetGLName();
	m_Api.DebugLabel(gl::kBuffer, bufferName, 0, name);
}

void GfxDeviceGLES::SetGpuProgramName(GpuProgram* prog, const char* name)
{
	Assert(prog);
	if(!prog || !prog->IsSupported())
		return;

	GlslGpuProgramGLES* glslProgram = reinterpret_cast<GlslGpuProgramGLES*>(prog);
	GLuint programName = glslProgram->GetProgramName();
	if(programName)
		m_Api.DebugLabel(gl::kProgram, programName, 0, name);
}

void GfxDeviceGLES::RegisterNativeTexture(TextureID texture, intptr_t nativeTex, TextureDimension dim)
{
	GLESTexture* texInfo = TextureIdMapGLES_QueryOrAlloc(texture);
	GLES_ASSERT(&m_Api, texInfo, "Invalid texture info");

	texInfo->texture = (GLuint)nativeTex;
	texInfo->dim = dim;
}

void GfxDeviceGLES::UnregisterNativeTexture(TextureID texture)
{
	GLESTexture* texInfo = (GLESTexture*) TextureIdMap::QueryNativeTexture(texture);
	if (!texInfo)
		return;

	TextureIdMap::RemoveTexture(texture);
	UNITY_DELETE(texInfo, kMemGfxDevice);
}

void* GfxDeviceGLES::GetNativeTexturePointer(TextureID id)
{
	GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(id);

	if (!texInfo)
		return NULL;

	return (void*)(intptr_t)texInfo->texture;
}

// -- New VBO/IBO functions --

GfxBuffer* GfxDeviceGLES::CreateIndexBuffer()
{
	IndexBufferGLES *buffer = UNITY_NEW(IndexBufferGLES, kMemGfxDevice);
	OnCreateBuffer(buffer);
	return buffer;
}

GfxBuffer* GfxDeviceGLES::CreateVertexBuffer()
{
	VertexBufferGLES *buffer = UNITY_NEW(VertexBufferGLES, kMemGfxDevice);
	OnCreateBuffer(buffer);
	return buffer;
}

void GfxDeviceGLES::DeleteBuffer(GfxBuffer* buffer)
{
	OnDeleteBuffer(buffer);
	UNITY_DELETE(buffer, kMemGfxDevice);
}

void GfxDeviceGLES::UpdateBuffer(GfxBuffer* buffer, GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data, UInt32 flags)
{
	AssertMsg(mode != kGfxBufferModeCircular || GetGraphicsCaps().gles.hasCircularBuffer, "Circular draw buffers are not supported");
	if (buffer->GetTarget() == kGfxBufferTargetIndex)
		static_cast<IndexBufferGLES*>(buffer)->Update(mode, label, size, data);
	else
		static_cast<VertexBufferGLES*>(buffer)->Update(mode, label, size, data);
}

void* GfxDeviceGLES::BeginBufferWrite(GfxBuffer* buffer, size_t offset, size_t size)
{
	if (buffer->GetTarget() == kGfxBufferTargetIndex)
		return static_cast<IndexBufferGLES*>(buffer)->BeginWrite(offset, size);
	else
		return static_cast<VertexBufferGLES*>(buffer)->BeginWrite(offset, size);
}

void GfxDeviceGLES::EndBufferWrite(GfxBuffer* buffer, size_t bytesWritten)
{
	if (buffer->GetTarget() == kGfxBufferTargetIndex)
		static_cast<IndexBufferGLES*>(buffer)->EndWrite(bytesWritten);
	else
		static_cast<VertexBufferGLES*>(buffer)->EndWrite(bytesWritten);
}

VertexDeclaration* GfxDeviceGLES::GetVertexDeclaration(const VertexChannelsInfo& declKey)
{
	return vertDeclCache.GetVertexDecl(declKey);
}

namespace {

static void UploadUniformMatrix4(ApiGLES & api, BuiltinShaderParamIndices::MatrixParamData& matParam, const GpuProgramParameters::ConstantBufferList* constantBuffers, const float* dataPtr, ConstantBuffersGLES& cbs)
{
	GLES_ASSERT(gGL, matParam.cols == 4 && matParam.rows == 4, "We only support 4x4 matrices");
	if (matParam.cbKey == 0)
	{
		if (matParam.isVectorized)
			GLES_CALL(gGL, glUniform4fv, matParam.gpuIndex, 4, dataPtr);
		else
			GLES_CALL(&api, glUniformMatrix4fv, matParam.gpuIndex, 1, GL_FALSE, dataPtr);
	}
	else if (constantBuffers != NULL)
	{
		for (size_t i = 0; i < constantBuffers->size(); ++i)
		{
			const GpuProgramParameters::ConstantBuffer& cb = (*constantBuffers)[i];
			if ((cb.m_Name.index | (cb.m_Size << 16)) == matParam.cbKey)
			{
				int idx = cbs.FindAndBindCB(cb.m_Name.index, cb.m_BindIndex, cb.m_Size);
				cbs.SetCBConstant(idx, matParam.gpuIndex, dataPtr, sizeof(Matrix4x4f));
				break;
			}
		}
	}
}

}//namespace

void GfxDeviceGLES::SetShaderPropertiesCopied(const ShaderPropertySheet& properties)
{
	if (properties.IsEmpty())
		return;

	if (m_State.activeProgram != NULL && m_State.activeProgramParams != NULL)
	{
		GpuProgramParameters::ParameterBuffer& buffer = GetParamsScratchBuffer();
		buffer.resize_uninitialized(0);
		m_State.activeProgramParams->PrepareOverridingValues(properties, buffer);
		GlslGpuProgramGLES& prog = static_cast<GlslGpuProgramGLES&>(*m_State.activeProgram);
		prog.ApplyGpuProgramGLES(*m_State.activeProgramParams, buffer.data()); // Why not use ApplyGpuProgram??
	}
}

void GfxDeviceGLES::BeforeDrawCall()
{
	DBG_LOG_GLES("BeforeDrawCall()");

	DeviceStateGLES &state = m_State;

	m_Context->GetFramebuffer().Prepare();

	UInt32 transformDirty = state.transformDirtyFlags;
	m_TransformState.UpdateWorldViewMatrix(m_BuiltinParamValues);

	// Set Unity built-in parameters
	{
		GLES_ASSERT(&m_Api, m_BuiltinParamIndices[kShaderVertex], "Invalid builtin uniforms");
		const BuiltinShaderParamIndices& params = *m_BuiltinParamIndices[kShaderVertex];

		// MVP matrix
		if (params.mat[kShaderInstanceMatMVP].gpuIndex >= 0 && (transformDirty & TransformState::kWorldViewProjDirty))
		{
			Matrix4x4f wvp;
			MultiplyMatrices4x4(&m_BuiltinParamValues.GetMatrixParam(kShaderMatProj), &m_TransformState.worldViewMatrix, &wvp);

			BuiltinShaderParamIndices::MatrixParamData matParam = params.mat[kShaderInstanceMatMVP];
			GLES_ASSERT(&m_Api, matParam.rows == 4 && matParam.cols == 4, "We only support 4x4 matrices");
			::UploadUniformMatrix4(m_Api, matParam, state.activeConstantBuffers, wvp.GetPtr(), state.constantBuffers);
		}
		// MV matrix
		if (params.mat[kShaderInstanceMatMV].gpuIndex >= 0 && (transformDirty & TransformState::kWorldViewDirty))
		{
			BuiltinShaderParamIndices::MatrixParamData matParam = params.mat[kShaderInstanceMatMV];
			GLES_ASSERT(&m_Api, matParam.rows == 4 && matParam.cols == 4, "We only support 4x4 matrices");
			::UploadUniformMatrix4(m_Api, matParam, state.activeConstantBuffers, m_TransformState.worldViewMatrix.GetPtr(), state.constantBuffers);
		}
		// Transpose of MV matrix
		if (params.mat[kShaderInstanceMatTransMV].gpuIndex >= 0 && (transformDirty & TransformState::kWorldViewDirty))
		{
			Matrix4x4f tWV;
			TransposeMatrix4x4(&m_TransformState.worldViewMatrix, &tWV);

			BuiltinShaderParamIndices::MatrixParamData matParam = params.mat[kShaderInstanceMatTransMV];
			GLES_ASSERT(&m_Api, matParam.rows == 4 && matParam.cols == 4, "We only support 4x4 matrices");
			::UploadUniformMatrix4(m_Api, matParam, state.activeConstantBuffers, tWV.GetPtr(), state.constantBuffers);
		}
		// Inverse transpose of MV matrix
		if (params.mat[kShaderInstanceMatInvTransMV].gpuIndex >= 0 && (transformDirty & TransformState::kWorldViewDirty))
		{
			const Matrix4x4f& mat = m_TransformState.worldViewMatrix;
			Matrix4x4f invWV, tInvWV;
			Matrix4x4f::Invert_General3D(mat, invWV);
			TransposeMatrix4x4(&invWV, &tInvWV);

			BuiltinShaderParamIndices::MatrixParamData matParam = params.mat[kShaderInstanceMatInvTransMV];
			GLES_ASSERT(&m_Api, matParam.rows == 4 && matParam.cols == 4, "We only support 4x4 matrices");
			::UploadUniformMatrix4(m_Api, matParam, state.activeConstantBuffers, tInvWV.GetPtr(), state.constantBuffers);
		}
		// M matrix
		if (params.mat[kShaderInstanceMatM].gpuIndex >= 0 && (transformDirty & TransformState::kWorldDirty))
		{
			BuiltinShaderParamIndices::MatrixParamData matParam = params.mat[kShaderInstanceMatM];
			const Matrix4x4f& mat = m_TransformState.worldMatrix;
			GLES_ASSERT(&m_Api, matParam.rows == 4 && matParam.cols == 4, "We only support 4x4 matrices");
			::UploadUniformMatrix4(m_Api, matParam, state.activeConstantBuffers, mat.GetPtr(), state.constantBuffers);
		}
		// Inverse M matrix
		if (params.mat[kShaderInstanceMatInvM].gpuIndex >= 0 && (transformDirty & TransformState::kWorldDirty))
		{
			BuiltinShaderParamIndices::MatrixParamData matParam = params.mat[kShaderInstanceMatInvM];
			const Matrix4x4f& mat = m_TransformState.worldMatrix;
			Matrix4x4f inverseMat;
			Matrix4x4f::Invert_General3D(mat, inverseMat);
			GLES_ASSERT(&m_Api, matParam.rows == 4 && matParam.cols == 4, "We only support 4x4 matrices");
			::UploadUniformMatrix4(m_Api, matParam, state.activeConstantBuffers, inverseMat.GetPtr(), state.constantBuffers);
		}
	}

	state.transformDirtyFlags &= ~TransformState::kWorldViewProjDirty;
	state.constantBuffers.UpdateBuffers();

	 // TODO: Theoretically could have image loads/stores without compute shaders but not caring.
	if(GetGraphicsCaps().hasComputeShader)
	{
		for (int i = 0; i <= m_State.randomWriteTargetMaxIndex; i++)
		{
			if (m_State.randomWriteTargetTextures[i].IsValid())
			{
				SetImageTexture(m_State.randomWriteTargetTextures[i], i);
			}
			else if (m_State.randomWriteTargetBuffers[i].IsValid() && i < m_State.activeProgramParams->GetBufferParams().size())
			{
				ComputeBufferCounter counter;
				const GpuProgramParameters::BufferParameter param = m_State.activeProgramParams->GetBufferParams()[i];
				counter.bindpoint = param.m_CounterIndex;
				counter.offset = param.m_CounterOffset;
				SetComputeBuffer(m_State.randomWriteTargetBuffers[i], i, counter, false, true);
			}
		}

	// Set required memory barriers as now all the resources have been set and we know
	// which bits are required
	DoMemoryBarriers();
}
}

void GfxDeviceGLES::DrawBuffers(GfxBuffer* indexBuf,
	const VertexStreamSource* vertexStreams, int vertexStreamCount,
	const DrawBuffersRange* drawRanges, int drawRangeCount,
	VertexDeclaration* vertexDecl, const ChannelAssigns& channels)
{
	VertexDeclarationGLES* vertexDeclaration = static_cast<VertexDeclarationGLES*>(vertexDecl);
	if (!vertexDeclaration)
		return;

	size_t vertexCount = 0;
	for (int r = 0; r < drawRangeCount; ++r)
	{
		const DrawBuffersRange& range = drawRanges[r];
		if (range.vertexCount > vertexCount)
			vertexCount = range.vertexCount;
	}

	bool hasDrawBaseVertex = GetGraphicsCaps().gles.hasDrawBaseVertex;
	if (hasDrawBaseVertex)
		SetVertexStateGLES(channels, vertexDeclaration, vertexStreams, 0, vertexStreamCount, vertexCount);

	BeforeDrawCall();

	UInt32 lastBaseVertex = 0xffffffff;

	IndexBufferGLES* indexBuffer = static_cast<IndexBufferGLES*>(indexBuf);
	
	for (int r = 0; r < drawRangeCount; ++r)
	{
		const DrawBuffersRange& range = drawRanges[r];

		// If we don't support baseVertex in DrawElements, we must rebind the vertex buffer each time with the correct offset, if it changes
		UInt32 drawBaseVertex = range.baseVertex;
		if (!hasDrawBaseVertex && lastBaseVertex != drawBaseVertex)
		{
			SetVertexStateGLES(channels, vertexDeclaration, vertexStreams, drawBaseVertex, vertexStreamCount, vertexCount);
			lastBaseVertex = range.baseVertex;
			drawBaseVertex = 0;
		}

		GLES_ASSERT(&m_Api, range.topology != kPrimitiveQuads, "Quads are not supported with OpenGL core or ES");

		if (!((GlslGpuProgramGLES *)m_State.activeProgram)->IsInputTopologyValid(range.topology))
		{
			LogRepeatingErrorString("The given primitive topology does not match with the topology expected by the geometry shader");
		}

		if (GetGraphicsCaps().gles.hasProgramPointSize && range.topology == kPrimitivePoints)
			m_Api.Enable(gl::kProgramPointSize);

		if (indexBuffer != NULL)
		{
			m_Api.BindElementArrayBuffer(indexBuffer->GetGLName());
			m_Api.DrawElements(range.topology, indexBuffer->GetBindPointer(range.firstIndexByte), range.indexCount, drawBaseVertex, range.instanceCount);
		}
		else
		{
			m_Api.DrawArrays(range.topology, range.firstVertex, range.vertexCount, range.instanceCount);
		}

		if (GetGraphicsCaps().gles.hasProgramPointSize && range.topology == kPrimitivePoints)
			m_Api.Disable(gl::kProgramPointSize);

		UInt32 multiplier = range.instanceCount == 0 ? 1 : range.instanceCount;
		int indexCount = indexBuffer != NULL ? range.indexCount : range.vertexCount;
		GetFrameStats().AddDrawCall(GetPrimitiveCount(indexCount, range.topology, false) * multiplier, range.vertexCount * multiplier, r > 0);
	}

	if (indexBuffer)
		indexBuffer->RecordRender();

	State().constantBuffers.RecordRender();

	for (int i = 0; i < vertexStreamCount; i++)
	{
		if (vertexStreams[i].buffer)
			((VertexBufferGLES*)vertexStreams[i].buffer)->RecordRender();
	}
}

void GfxDeviceGLES::Clear(UInt32 clearFlags, const ColorRGBAf& color, float depth, UInt32 stencil)
{
	DBG_LOG_GLES("Clear(%d, (%.2f, %.2f, %.2f, %.2f), %.2f, %d", clearFlags, color.r, color.g, color.g, color.a, depth, stencil);

	m_Context->GetFramebuffer().Clear(clearFlags, color, depth, stencil, true);
}

void GfxDeviceGLES::SetSurfaceFlags(RenderSurfaceHandle surf, UInt32 flags, UInt32 keepFlags)
{
}

void GfxDeviceGLES::SetStereoTarget(StereoscopicEye eye)
{
	GLES_ASSERT(&m_Api, GetGraphicsCaps().hasStereoscopic3D, "Quad stereo rendering not supported");

	if (eye == kStereoscopicEyeLeft)
		m_Api.BindFramebufferDrawBuffers(GetFramebuffer().GetDefaultFBO(), 1, &GL_BACK_LEFT);
	else
		m_Api.BindFramebufferDrawBuffers(GetFramebuffer().GetDefaultFBO(), 1, &GL_BACK_RIGHT);
}

void GfxDeviceGLES::ReloadResources()
{
	// Buffers in BufferManager must be cleared before recreating VBOs.
	GetBufferManagerGLES()->InvalidateAll();
	// Clear all TF shaders we may have
	TransformFeedbackSkinning::CleanupTransformFeedbackShaders();
#if GFX_BUFFERS_CAN_BECOME_LOST
	ResetAllBuffers();
#endif

	DeleteDynamicVBO();
	GetDynamicVBO();

	GfxDevice::CommonReloadResources(kReleaseRenderTextures | kReloadShaders | kReloadTextures);

	m_Context->Invalidate(m_Api);

	InvalidateState();
}

RenderTextureFormat GfxDeviceGLES::GetDefaultRTFormat() const
{
	return GetGraphicsCaps().gles.defaultRenderTextureFormat;
}

RenderTextureFormat GfxDeviceGLES::GetDefaultHDRRTFormat() const
{
	if (GetGraphicsCaps().supportsRenderTextureFormat[kRTFormatARGBHalf])
		return kRTFormatARGBHalf;
	else
		return GetGraphicsCaps().gles.defaultRenderTextureFormat;
}

// -- Compute shaders --

class ComputeBufferGLES
{
public:
	DataBufferGLES*		m_Buffer;
	size_t				m_Capacity;
	size_t				m_Stride;
	UInt32				m_Flags;
	BarrierTime			m_WriteTime;

	DataBufferGLES*		m_CounterBacking; // Persistent storage for append/consume counter
	int					m_CounterIndex; // Binding point (atm should always be 0)
	int					m_CounterOffset; // Offset at the atomic counter buffer
	int					m_PrevCounterSlot; // Position at which the counter was previously
	BarrierTime			m_CounterWriteTime;
};

// Mark a memory barrier required before next draw/dispatch call if the resource has been
// asynchronously written to since previous barrier
void GfxDeviceGLES::MemoryBarrierBeforeDraw(BarrierTime previousWriteTime, gl::MemoryBarrierType type)
{
	if (m_State.barrierTimes[(int)type] < previousWriteTime)
		m_State.requiredBarriers |= m_Api.translate.MemoryBarrierBits(type);
}

// Call a memory barrier immediately if the resource has been asynchronously written to
// since the previous barrier
void GfxDeviceGLES::MemoryBarrierImmediate(BarrierTime previousWriteTime, gl::MemoryBarrierType type)
{
	if (m_State.barrierTimes[(int)type] < previousWriteTime)
	{
		GLES_CALL(&m_Api, glMemoryBarrier, m_Api.translate.MemoryBarrierBits(type));
		m_State.barrierTimes[(int)type] = m_State.barrierTimeCounter++; // Mark barrier time
		m_State.requiredBarriers &= ~m_Api.translate.MemoryBarrierBits(type); // Clear this barrier bit from the required list
	}
}

static const GLbitfield kMemoryBarrierMaskDefaultDraw =
	GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT |
	GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT |
	GL_TRANSFORM_FEEDBACK_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT;

static const GLbitfield kMemoryBarrierMaskIndirectDraw = kMemoryBarrierMaskDefaultDraw | GL_COMMAND_BARRIER_BIT;

static const GLbitfield kMemoryBarrierMaskDispatchCompute =
	GL_UNIFORM_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
	GL_TRANSFORM_FEEDBACK_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT;

// Sets a mask to use only a subset of required barrier bits for the next DoMemoryBarriers.
// DoMemoryBarriers will reset the mask to default afterwards.
void GfxDeviceGLES::SetBarrierMask(GLbitfield mask)
{
	m_State.requiredBarriersMask = mask;
}

// Call a memory barrier with bits recognized as required and clear the bits afterwards
void GfxDeviceGLES::DoMemoryBarriers()
{
	if (m_State.requiredBarriers)
	{
		if(m_State.requiredBarriers & m_State.requiredBarriersMask)
			GLES_CALL(&m_Api, glMemoryBarrier, m_State.requiredBarriers & m_State.requiredBarriersMask);

		// Mark the time and zero the bit for the set barriers
		for (int i = 0; i < gl::kBarrierTypeCount; i++)
		{
			if (m_State.requiredBarriers & m_State.requiredBarriersMask &
				m_Api.translate.MemoryBarrierBits((gl::MemoryBarrierType)i))
			{
				m_State.barrierTimes[i] = m_State.barrierTimeCounter;
				m_State.requiredBarriers &= ~m_Api.translate.MemoryBarrierBits((gl::MemoryBarrierType)i);
			}
		}

		m_State.barrierTimeCounter++;
		m_State.requiredBarriersMask = kMemoryBarrierMaskDefaultDraw; // reset mask to default draw
	}
}

static ComputeBufferGLES *GetComputeBufferGLES(ComputeBufferID bufferHandle, std::map<ComputeBufferID, ComputeBufferGLES *> &bufMap)
{
	if (!bufferHandle.IsValid())
		return 0;

	std::map<ComputeBufferID, ComputeBufferGLES *>::iterator itr = bufMap.find(bufferHandle);
	if (itr == bufMap.end())
		return 0;

	return itr->second;
}

void GfxDeviceGLES::SetComputeBufferCounterValue(ComputeBufferID bufferHandle, UInt32 value)
{
	ComputeBufferGLES *buf = GetComputeBufferGLES(bufferHandle, m_ComputeBuffers);
	if (!buf)
		return;

	DataBufferGLES* counterBuf = buf->m_CounterBacking;
	if (counterBuf)
	{
		counterBuf->Upload(0, sizeof(UInt32), &value);
	}
	// If the buffer is bound, also update the atomic counter buffer
	if (buf->m_PrevCounterSlot >= 0 && m_AtomicCounterSlots[buf->m_PrevCounterSlot] == buf)
	{
		m_AtomicCounterBuffer->Upload(buf->m_PrevCounterSlot * sizeof(GLuint), sizeof(GLuint), &value);
	}
}

void GfxDeviceGLES::SetComputeBuffer(ComputeBufferID bufferHandle, int index, ComputeBufferCounter counter, bool recordRender, bool recordUpdate)
{
	ComputeBufferGLES *buf = GetComputeBufferGLES(bufferHandle, m_ComputeBuffers);
	if (!buf)
		return;

	// Index 0x7FFFFFFF here indicates no buffer is actually present in gles, but counter is still needed
	if (index != 0x7FFFFFFF)
		m_Api.BindShaderStorageBuffer(index, buf->m_Buffer->GetBuffer());

	if (recordRender)
		buf->m_Buffer->RecordRender();

	MemoryBarrierBeforeDraw(buf->m_WriteTime, gl::kBarrierShaderStorage);

	if (buf->m_Flags & kCBFlagIndirectArguments)
		MemoryBarrierBeforeDraw(buf->m_WriteTime, gl::kBarrierCommand);

	if (recordUpdate)
	{
		buf->m_WriteTime = m_State.barrierTimeCounter + 1; // Mark new buffer write time for barrier resolving
		buf->m_Buffer->RecordUpdate();
	}

	// Handle append/consume counter if present
	if (counter.bindpoint >= 0 && counter.offset >= 0)
	{
		buf->m_CounterIndex = counter.bindpoint;
		buf->m_CounterOffset = counter.offset;

		// Reserve the global buffer for counters in the first run
		if (!m_AtomicCounterBuffer)
			m_AtomicCounterBuffer = GetBufferManagerGLES()->AcquireBuffer(kMaxAtomicCounters * sizeof(GLuint), DataBufferGLES::kDynamicACBO, true);

		// We use slot array to mark which offsets are used for which buffer's counter
		int slotIndex = counter.offset / sizeof(GLuint);
		
		// Evict the previous data from the slot
		if (m_AtomicCounterSlots[slotIndex] != NULL && m_AtomicCounterSlots[slotIndex] != buf)
		{
			//TODO: should this actually use gl::kBarrierBufferUpdate bit?
			MemoryBarrierImmediate(m_AtomicCounterSlots[slotIndex]->m_CounterWriteTime, gl::kBarrierAtomicCounter);
			m_AtomicCounterSlots[slotIndex]->m_CounterBacking->CopySubData(m_AtomicCounterBuffer, counter.offset, 0, sizeof(GLuint));
			m_AtomicCounterSlots[slotIndex] = NULL;
		}

		{
			// Copy data only if the slot has changed
			if (m_AtomicCounterSlots[slotIndex] != buf)
			{
				MemoryBarrierImmediate(buf->m_CounterWriteTime, gl::kBarrierAtomicCounter);
				// Copy counter either from alive previous slot
				if (buf->m_PrevCounterSlot >= 0 && m_AtomicCounterSlots[buf->m_PrevCounterSlot] == buf)
					m_AtomicCounterBuffer->CopySubData(m_AtomicCounterBuffer, buf->m_PrevCounterSlot * sizeof(GLuint), counter.offset, sizeof(GLuint));
				else // or from backing
					m_AtomicCounterBuffer->CopySubData(buf->m_CounterBacking, 0, counter.offset, sizeof(GLuint));
			}
			else // Same slot -> no need to move data around. Just add barrier to make sure ops from prev shader invocation have finished.
				MemoryBarrierBeforeDraw(buf->m_CounterWriteTime, gl::kBarrierAtomicCounter);
		}

		buf->m_CounterWriteTime = m_State.barrierTimeCounter;

		// Clear the previous slot used for this counter
		if (buf->m_PrevCounterSlot >= 0 && slotIndex != buf->m_PrevCounterSlot
			&& m_AtomicCounterSlots[buf->m_PrevCounterSlot] == buf)
			m_AtomicCounterSlots[buf->m_PrevCounterSlot] = NULL;

		// Mark the currently used slot
		m_AtomicCounterSlots[slotIndex] = buf;
		buf->m_PrevCounterSlot = slotIndex;

		m_Api.BindAtomicCounterBuffer(counter.bindpoint, m_AtomicCounterBuffer->GetBuffer());
	}
}

void GfxDeviceGLES::SetComputeBufferData(ComputeBufferID bufferHandle, const void* data, size_t size)
{
	ComputeBufferGLES *buf = GetComputeBufferGLES(bufferHandle, m_ComputeBuffers);
	if (!buf)
		return;

	if (BufferUpdateCausesStallGLES(buf->m_Buffer) || buf->m_Buffer->GetSize() < size)
	{
		buf->m_Buffer->Release();
		buf->m_Buffer = GetBufferManagerGLES()->AcquireBuffer(size, DataBufferGLES::kDynamicSSBO);
	}
	else
	{
		// When we update a previously written buffer a proper memory barrier must be set
		MemoryBarrierImmediate(buf->m_WriteTime, gl::kBarrierBufferUpdate);
	}

	buf->m_Buffer->Upload(0, size, data); // Implicitly does RecordUpdate

	buf->m_WriteTime = m_State.barrierTimeCounter;
	MemoryBarrierBeforeDraw(buf->m_WriteTime, gl::kBarrierBufferUpdate);
}

void GfxDeviceGLES::GetComputeBufferData(ComputeBufferID bufferHandle, void* dest, size_t destSize)
{
	ComputeBufferGLES *buf = GetComputeBufferGLES(bufferHandle, m_ComputeBuffers);
	if (!buf)
		return;

	// Reading a previously written buffer also requires a barrier
	MemoryBarrierImmediate(buf->m_WriteTime, gl::kBarrierBufferUpdate);

	void *mapped = buf->m_Buffer->Map(0, destSize, GL_MAP_READ_BIT);
	if (!mapped)
		return;
	::memcpy(dest, mapped, destSize);
	buf->m_Buffer->Unmap();
}

void GfxDeviceGLES::CopyComputeBufferCount(ComputeBufferID srcBuffer, ComputeBufferID dstBuffer, UInt32 dstOffset)
{
	ComputeBufferGLES *src = GetComputeBufferGLES(srcBuffer, m_ComputeBuffers);
	ComputeBufferGLES *dst = GetComputeBufferGLES(dstBuffer, m_ComputeBuffers);
	if (!src || !dst || src->m_CounterIndex < 0 || !m_AtomicCounterBuffer)
		return;

	// We must set a barrier before copy if either src or dst has been written into.
	// src->m_writeTime is effectively correct since writes to its counter buffer happen at the same time
	MemoryBarrierImmediate(src->m_WriteTime, gl::kBarrierBufferUpdate);
	MemoryBarrierImmediate(src->m_WriteTime, gl::kBarrierAtomicCounter);
	MemoryBarrierImmediate(dst->m_WriteTime, gl::kBarrierBufferUpdate);

	// Copy either from atomic counter buffer or the counter backing buf
	if (m_AtomicCounterSlots[src->m_PrevCounterSlot] == src)
		dst->m_Buffer->CopySubData(m_AtomicCounterBuffer, src->m_CounterOffset, dstOffset, sizeof(GLuint));
	else
		dst->m_Buffer->CopySubData(src->m_CounterBacking, 0, dstOffset, sizeof(GLuint));
}

void GfxDeviceGLES::SetImageTexture(TextureID tid, int index)
{
	if (!tid.IsValid())
		return;

	GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(tid);

	MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierShaderImageAccess);
	texInfo->imageWriteTime = m_State.barrierTimeCounter + 1; // atm whenever image texture is being set, we presume that it is being written into

	Assert(texInfo->internalFormat);
	GLES_CALL(&m_Api, glBindImageTexture, index, texInfo->texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, texInfo->internalFormat);
}

void GfxDeviceGLES::SetRandomWriteTargetTexture(int index, TextureID tid)
{
	if (index >= 0 && index < kMaxSupportedRenderTargets)
	{
		m_State.randomWriteTargetMaxIndex = std::max(m_State.randomWriteTargetMaxIndex, index);
		m_State.randomWriteTargetTextures[index] = tid;
		m_State.randomWriteTargetBuffers[index] = ComputeBufferID();
	}
	else
	{
		GLES_ASSERT(&m_Api, index >= 0 && index < kMaxSupportedRenderTargets, "Invalid RenderTarget");
		WarningString(Format("Random write target index out of bounds"));
	}
}

void GfxDeviceGLES::SetRandomWriteTargetBuffer(int index, ComputeBufferID bufferHandle)
{
	if (index >= 0 && index < kMaxSupportedRenderTargets)
	{
		m_State.randomWriteTargetMaxIndex = std::max(m_State.randomWriteTargetMaxIndex, index);
		m_State.randomWriteTargetBuffers[index] = bufferHandle;
		m_State.randomWriteTargetTextures[index].m_ID = 0;
	}
	else
	{
		GLES_ASSERT(&m_Api, index >= 0 && index < kMaxSupportedRenderTargets, "Invalid RenderTarget");
		WarningString(Format("Random write target index out of bounds"));
	}
}

void GfxDeviceGLES::ClearRandomWriteTargets()
{
	for (int i = 0; i <= m_State.randomWriteTargetMaxIndex; i++)
	{
		if (m_State.randomWriteTargetTextures[i].IsValid())
		{
			m_State.randomWriteTargetTextures[i].m_ID = 0;
		}
		else if (m_State.randomWriteTargetBuffers[i].IsValid())
		{
			m_State.randomWriteTargetBuffers[i] = ComputeBufferID();
		}
	}

	m_State.randomWriteTargetMaxIndex = -1;
}

bool GfxDeviceGLES::HasActiveRandomWriteTarget() const
{
	return m_State.randomWriteTargetMaxIndex != -1;
}

struct ComputeProgramGLES
{
	GLuint program;
};


ComputeProgramHandle GfxDeviceGLES::CreateComputeProgram(const UInt8* code, size_t)
{
	ComputeProgramHandle cph;
	cph.object = 0;
	if (!GetGraphicsCaps().hasComputeShader)
	{
		return cph;
	}

	const char *sources[1] = { (const char *)code };

	GLuint shader = m_Api.CreateShader(gl::kComputeShaderStage, sources[0]);
	if (!m_Api.CheckShader(shader))
	{
		m_Api.DeleteShader(shader);
		return cph;
	}

	GLuint program = m_Api.CreateComputeProgram(shader);

	if (!m_Api.CheckProgram(program))
	{
		ErrorString(Format("ERROR: Unable to link compute shader!"));
		m_Api.DeleteProgram(program);
		return cph;
	}

	ComputeProgramGLES *object = new ComputeProgramGLES();
	object->program = program;
	cph.object = (void *)object;
	return cph;
}

void GfxDeviceGLES::DestroyComputeProgram(ComputeProgramHandle& cpHandle)
{
	if (!cpHandle.IsValid())
		return;

	ComputeProgramGLES *object = (ComputeProgramGLES *)cpHandle.object;
	m_Api.DeleteProgram(object->program);

	delete object;
	cpHandle.object = 0;
}

void  GfxDeviceGLES::ResolveComputeProgramResources(ComputeProgramHandle cpHandle, ComputeShaderKernel& kernel, std::vector<ComputeShaderCB>& constantBuffers, std::vector<ComputeShaderParam>& uniforms, bool preResolved)
{
	if (!cpHandle.IsValid())
		return;

	if (!preResolved)
	{
		kernel.textures.clear();
		kernel.builtinSamplers.clear();
		kernel.inBuffers.clear();
		kernel.outBuffers.clear();
	}

	kernel.cbs.clear();

	GLuint program = ((ComputeProgramGLES *)cpHandle.object)->program;
	gles::UseGLSLProgram(m_State, program);
	GpuProgramParameters params;
	PropertyNamesSet propNames;
	FillParamsBaseGLES(program, params, &propNames);
	GLES_CALL(gGL, glGetProgramiv, program, GL_COMPUTE_WORK_GROUP_SIZE, reinterpret_cast<GLint*>(kernel.threadGroupSize));

	//	This used to be only for not preresolved stuff, but we'll need it for constant buffer layouts anyway.
	{
		// Uniforms
		GpuProgramParameters::ValueParameterArray::const_iterator valueParamsEnd = params.GetValueParams().end();
		for (GpuProgramParameters::ValueParameterArray::const_iterator i = params.GetValueParams().begin(); i != valueParamsEnd; ++i)
		{
			ComputeShaderParam uniform;
			uniform.name = i->m_Name;
			uniform.type = i->m_Type;
			uniform.offset = i->m_Index;
			uniform.arraySize = i->m_ArraySize;
			uniform.rowCount = i->m_RowCount;
			uniform.colCount = i->m_ColCount;

			uniforms.push_back(uniform);
		}

		// Constant buffers
		GpuProgramParameters::ConstantBufferList::const_iterator constantBuffersEnd = params.GetConstantBuffers().end();
		for (GpuProgramParameters::ConstantBufferList::const_iterator i = params.GetConstantBuffers().begin(); i != constantBuffersEnd; ++i)
		{
			ComputeShaderCB newConstantBuffer;
			newConstantBuffer.name = i->m_Name;
			newConstantBuffer.byteSize = i->m_Size;
			newConstantBuffer.params.clear();

			ComputeShaderCB &constantBuffer = FindOrAddByName(constantBuffers, newConstantBuffer);

			// CBs are shared with all kernels within a .compute -> CB might be in the vector already
			if (constantBuffer.params.size() == 0)
			{
				// Value params in current cb
				GpuProgramParameters::ValueParameterArray::const_iterator cbParamsEnd = i->m_ValueParams.end();
				for (GpuProgramParameters::ValueParameterArray::const_iterator j = i->m_ValueParams.begin(); j != cbParamsEnd; ++j)
				{
					ComputeShaderParam param;
					param.name = j->m_Name;
					param.type = j->m_Type;
					param.offset = j->m_Index;
					param.arraySize = j->m_ArraySize;
					param.rowCount = j->m_RowCount;
					param.colCount = j->m_ColCount;
					constantBuffer.params.push_back(param);
				}
			}

			// add to current kernel's cb resource list
			ComputeShaderResource newRes;
			newRes.name = i->m_Name;
			newRes.bindPoint = i->m_BindIndex;
			kernel.cbs.push_back(newRes);
		}
	}

	// Compute buffers (SSBO)
	GpuProgramParameters::BufferParameterArray::const_iterator bufferParamsEnd = params.GetBufferParams().end();
	for (GpuProgramParameters::BufferParameterArray::const_iterator i = params.GetBufferParams().begin(); i != bufferParamsEnd; ++i)
	{
		if (preResolved)
		{
			//TODO: resolve these in HLSLcc/import time to avoid searches
			bool found = false;

			for (size_t b = 0, nb = kernel.inBuffers.size(); b < nb && !found; ++b)
			{
				if (kernel.inBuffers[b].name == i->m_Name)
				{
					kernel.inBuffers[b].bindPoint = i->m_Index;

					if (i->m_CounterIndex >= 0) // need to get the buffer binding for counter if present
					{
						kernel.inBuffers[b].counter.bindpoint = i->m_CounterIndex;
						kernel.inBuffers[b].counter.offset = i->m_CounterOffset;
					}
					else
					{
						kernel.inBuffers[b].counter.bindpoint = -1;
						kernel.inBuffers[b].counter.offset = -1;
					}

					found = true;
				}
			}

			for (size_t b = 0, nb = kernel.outBuffers.size(); b < nb && !found; ++b)
			{
				if (kernel.outBuffers[b].name == i->m_Name)
				{
					kernel.outBuffers[b].bindPoint = i->m_Index;

					if (i->m_CounterIndex >= 0) // need to get the buffer binding for counter if present
					{
						kernel.outBuffers[b].counter.bindpoint = i->m_CounterIndex;
						kernel.outBuffers[b].counter.offset = i->m_CounterOffset;
					}
					else
					{
						kernel.outBuffers[b].counter.bindpoint = -1;
						kernel.outBuffers[b].counter.offset = -1;
					}

					found = true;
				}
			}
		}
		else
		{
			ComputeShaderResource buffer;
			buffer.name = i->m_Name;
			buffer.bindPoint = i->m_Index;
			buffer.counter.bindpoint = i->m_CounterIndex;
			buffer.counter.offset = i->m_CounterOffset;

			kernel.inBuffers.push_back(buffer); //TODO: in vs out buffers?
		}
	}

	// Textures
	if (preResolved)
	{
		for (int t = (int)kernel.textures.size() - 1; t >= 0; t--) // backwards loop for easy element removing
		{
			bool paramFound = false;

			GpuProgramParameters::TextureParameterList::const_iterator textureParamsEnd = params.GetTextureParams().end();
			for (GpuProgramParameters::TextureParameterList::const_iterator i = params.GetTextureParams().begin(); i != textureParamsEnd; ++i)
			{
				if (kernel.textures[t].generatedName == i->m_Name)
				{
					kernel.textures[t].bindPoint = (kernel.textures[t].bindPoint & 0xFFFF0000) | i->m_Index;
					kernel.builtinSamplers[t].bindPoint = i->m_Index;
					paramFound = true;
					break;
				}
			}

			if (!paramFound) // Not found in params means it is declared but not used -> remove from the list
			{				 // TODO: Check if these could be recognized earlier (in HLSLcc)
				kernel.textures.erase(kernel.textures.begin() + t);
				kernel.builtinSamplers.erase(kernel.builtinSamplers.begin() + t);
			}
		}
	}
	else
	{
		GpuProgramParameters::TextureParameterList::const_iterator textureParamsEnd = params.GetTextureParams().end();
		for (GpuProgramParameters::TextureParameterList::const_iterator i = params.GetTextureParams().begin(); i != textureParamsEnd; ++i)
		{
			ComputeShaderResource texture;
			texture.name = i->m_Name;
			texture.bindPoint = i->m_Index;
			kernel.textures.push_back(texture);

			// No built-in samplers on hand-written GLSL. Need matching element for each texture though.
			ComputeShaderBuiltinSampler sampler;
			sampler.sampler = kBuiltinSamplerStateCount;
			sampler.bindPoint = 0;
			kernel.builtinSamplers.push_back(sampler);
		}
	}

	// Images
	GpuProgramParameters::ImageParameterArray::const_iterator imageParamsEnd = params.GetImageParams().end();
	for (GpuProgramParameters::ImageParameterArray::const_iterator i = params.GetImageParams().begin(); i != imageParamsEnd; ++i)
	{
		if (preResolved)
		{
			for (size_t b = 0, nb = kernel.outBuffers.size(); b < nb; ++b)
			{
				if (kernel.outBuffers[b].name == i->m_Name)
				{
					kernel.outBuffers[b].bindPoint = i->m_Index;
					break;
				}
			}
		}
		else
		{
			ComputeShaderResource image;
			image.name = i->m_Name;
			image.bindPoint = i->m_Index;
			kernel.outBuffers.push_back(image);
		}
	}
}

void GfxDeviceGLES::CreateComputeConstantBuffers(unsigned count, const UInt32* sizes, ConstantBufferHandle* outCBs)
{
	if (!GetGraphicsCaps().hasComputeShader)
	{
		for (unsigned i = 0; i < count; i++)
			outCBs[i].object = 0;
		return;
	}

	BufferManagerGLES *mgr = GetBufferManagerGLES();
	for (unsigned i = 0; i < count; i++)
	{
		// reuse same ID space as compute buffers. These IDs are guaranteed to start from 1 so
		// are safe to reuse as pointers (ie. IsValid() will still work as expected on ComputeBufferHandle)
		ComputeBufferID id = CreateComputeBufferID();
		
		// TODO: matches DX11 usage now, but should we expose static/dynamic in flags?
		m_ComputeConstantBuffers.insert(std::make_pair(id, mgr->AcquireBuffer(sizes[i], DataBufferGLES::kDynamicUBO)));

		outCBs[i].object = (void *)id.m_ID;
	}
}

void GfxDeviceGLES::DestroyComputeConstantBuffers(unsigned count, ConstantBufferHandle* cbs)
{
	for (unsigned i = 0; i < count; i++)
	{
		if (!cbs[i].IsValid())
			continue;

		ComputeBufferID id((GfxResourceIDType)reinterpret_cast<uintptr_t>(cbs[i].object));

		std::map<ComputeBufferID, DataBufferGLES *>::iterator itr = m_ComputeConstantBuffers.find(id);
		if (itr == m_ComputeConstantBuffers.end())
			continue;

		DataBufferGLES *buf = itr->second;
		if (buf)
			buf->Release();
		cbs[i].Reset();
		m_ComputeConstantBuffers.erase(itr);
	}
}

void GfxDeviceGLES::CreateComputeBuffer(ComputeBufferID id, size_t count, size_t stride, UInt32 flags)
{
	if (!GetGraphicsCaps().hasComputeShader)
	{
		id.m_ID = 0;
		return;
	}

	DataBufferGLES *buf = GetBufferManagerGLES()->AcquireBuffer(count * stride, DataBufferGLES::kDynamicSSBO, true);

	DataBufferGLES *counterBuf = NULL;
	if ((flags & kCBFlagAppend) || (flags & kCBFlagCounter))
	{
		counterBuf = GetBufferManagerGLES()->AcquireBuffer(sizeof(GLuint), DataBufferGLES::kDynamicSSBO, true);
	}

	ComputeBufferGLES *cbuf = UNITY_NEW(ComputeBufferGLES, kMemGfxDevice);

	cbuf->m_Buffer = buf;
	cbuf->m_CounterBacking = counterBuf;
	cbuf->m_CounterIndex = -1;
	cbuf->m_CounterOffset = 0;
	cbuf->m_Capacity = count;
	cbuf->m_Stride = stride;
	cbuf->m_Flags = flags;
	cbuf->m_WriteTime = 0;
	cbuf->m_CounterWriteTime = 0;
	cbuf->m_PrevCounterSlot = -1;

	m_ComputeBuffers.insert(std::make_pair(id, cbuf));
}

void GfxDeviceGLES::DestroyComputeBuffer(ComputeBufferID handle)
{
	std::map<ComputeBufferID, ComputeBufferGLES *>::iterator itr = m_ComputeBuffers.find(handle);
	if (itr == m_ComputeBuffers.end())
		return;

	ComputeBufferGLES *cbuf = itr->second;
	if (cbuf->m_Buffer)
		cbuf->m_Buffer->Release();
	if (cbuf->m_CounterBacking)
		cbuf->m_CounterBacking->Release();
	if (m_AtomicCounterSlots[cbuf->m_PrevCounterSlot] == cbuf)
		m_AtomicCounterSlots[cbuf->m_PrevCounterSlot] = NULL;
	UNITY_DELETE(cbuf, kMemGfxDevice);

	m_ComputeBuffers.erase(itr);
}

typedef void(*ProgramUniformFunc)(const ApiGLES* api, GLuint program, GLint location, GLsizei count, GLboolean transpose, const void* value);
static void ProgramUniform1fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform1fv, p, l, c, (GLfloat*)v); }
static void ProgramUniform2fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform2fv, p, l, c, (GLfloat*)v); }
static void ProgramUniform3fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform3fv, p, l, c, (GLfloat*)v); }
static void ProgramUniform4fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform4fv, p, l, c, (GLfloat*)v); }
static void ProgramUniform1iv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform1iv, p, l, c, (GLint*)v); }
static void ProgramUniform2iv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform2iv, p, l, c, (GLint*)v); }
static void ProgramUniform3iv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform3iv, p, l, c, (GLint*)v); }
static void ProgramUniform4iv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform4iv, p, l, c, (GLint*)v); }
static void ProgramUniform1uiv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform1uiv, p, l, c, (GLuint*)v); }
static void ProgramUniform2uiv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform2uiv, p, l, c, (GLuint*)v); }
static void ProgramUniform3uiv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform3uiv, p, l, c, (GLuint*)v); }
static void ProgramUniform4uiv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniform4uiv, p, l, c, (GLuint*)v); }
static void ProgramUniformMatrix2fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix2fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix3fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix3fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix4fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix4fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix2x3fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix2x3fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix3x2fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix3x2fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix2x4fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix2x4fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix4x2fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix4x2fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix3x4fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix3x4fv, p, l, c, t, (GLfloat*)v); }
static void ProgramUniformMatrix4x3fv(const ApiGLES* a, GLuint p, GLint l, GLsizei c, GLboolean t, const void* v) { GLES_CALL(a, glProgramUniformMatrix4x3fv, p, l, c, t, (GLfloat*)v); }

// ProgramUniform*fv funcs arranged by [cols][rows]
ProgramUniformFunc floatUniformFuncs[4][4] = {
	{ ProgramUniform1fv,	ProgramUniform2fv,			ProgramUniform3fv,			ProgramUniform4fv },
	{ NULL,					ProgramUniformMatrix2fv,	ProgramUniformMatrix2x3fv,	ProgramUniformMatrix2x4fv},
	{ NULL,					ProgramUniformMatrix3x2fv,	ProgramUniformMatrix3fv,	ProgramUniformMatrix3x4fv },
	{ NULL,					ProgramUniformMatrix4x2fv,	ProgramUniformMatrix4x3fv,	ProgramUniformMatrix4fv }
};

// ProgramUniform*(u)iv funcs
ProgramUniformFunc intUniformFuncs[4] = { ProgramUniform1iv, ProgramUniform2iv, ProgramUniform3iv, ProgramUniform4iv };
ProgramUniformFunc uintUniformFuncs[4] = { ProgramUniform1uiv, ProgramUniform2uiv, ProgramUniform3uiv, ProgramUniform4uiv };


void GfxDeviceGLES::SetComputeUniform(ComputeProgramHandle cpHandle, ComputeShaderParam& uniform, size_t byteCount, const void* data)
{
	GLuint program = ((ComputeProgramGLES *)cpHandle.object)->program;
	size_t numElementsExpected = uniform.rowCount * uniform.colCount * uniform.arraySize;
	size_t numElements = byteCount/4;
	ProgramUniformFunc func = NULL;

	if (uniform.type == kShaderParamFloat)
	{
		GLES_ASSERT(&m_Api, uniform.colCount >= 1 && uniform.colCount <= 4, "Invalid count of components");
		GLES_ASSERT(&m_Api, uniform.rowCount >= 1 && uniform.rowCount <= 4, "Invalid count of components");

		func = floatUniformFuncs[uniform.colCount - 1][uniform.rowCount - 1];
	}
	else if (uniform.type == kShaderParamInt)
	{
		GLES_ASSERT(&m_Api, uniform.colCount == 1, "Invalid count of components");
		GLES_ASSERT(&m_Api, uniform.rowCount >= 1 && uniform.rowCount <= 4, "Invalid count of components");

		func = intUniformFuncs[uniform.rowCount - 1];
	}
	else
	{
		GLES_ASSERT(&m_Api, uniform.colCount == 1, "Invalid count of components");
		GLES_ASSERT(&m_Api, uniform.rowCount >= 1 && uniform.rowCount <= 4, "Invalid count of components");
		GLES_ASSERT(&m_Api, uniform.type == kShaderParamUInt, "Invalid count of components");

		func = uintUniformFuncs[uniform.rowCount - 1];
	}

	if (numElements > numElementsExpected)
	{
		WarningString(Format("ComputeShader: Trying to set uniform with %i elements whereas only %i were expected. The excess data is being discarded.", numElements, numElementsExpected));
	}
	else if (numElements < numElementsExpected)
	{
		ErrorString(Format("ComputeShader: Trying to set uniform with %i elements whereas %i were expected. Could not set uniform.", numElements, numElementsExpected));
		return;
	}

	GLES_ASSERT(&m_Api, func != NULL, "Fail to select the glProgramUniform function");
	func(&m_Api, program, uniform.offset, uniform.arraySize, GL_FALSE, data);
}

void GfxDeviceGLES::UpdateComputeConstantBuffers(unsigned count, ConstantBufferHandle* cbs, UInt32 cbDirty, size_t dataSize, const UInt8* data, const UInt32* cbSizes, const UInt32* cbOffsets, const int* bindPoints)
{
	for (unsigned i = 0; i < count; ++i)
	{
		if (bindPoints[i] < 0)
			continue; // CB not going to be used, no point in updating it

		ComputeBufferID id((GfxResourceIDType)reinterpret_cast<uintptr_t>(cbs[i].object));

		std::map<ComputeBufferID, DataBufferGLES *>::iterator itr = m_ComputeConstantBuffers.find(id);
		if (itr == m_ComputeConstantBuffers.end())
			continue;
		
		DataBufferGLES *cb = itr->second;
		
		UInt32 dirtyMask = (1 << i);
		if (cbDirty & dirtyMask)
		{
			if (BufferUpdateCausesStallGLES(cb))
			{
				cb->Release();
				cb = GetBufferManagerGLES()->AcquireBuffer(cbSizes[i], DataBufferGLES::kDynamicUBO);
				itr->second = cb;
			}
			
			cb->Upload(0, cbSizes[i], data + cbOffsets[i]); // Also does RecordUpdate
		}

		// bind it
		m_Api.BindUniformBuffer(bindPoints[i], cb->GetBuffer());
	}
}

void GfxDeviceGLES::UpdateComputeResources(
	unsigned texCount, const TextureID* textures, const TextureDimension* texDims, const int* texBindPoints,
	unsigned samplerCount, const unsigned* samplers,
	unsigned inBufferCount, const ComputeBufferID* inBuffers, const int* inBufferBindPoints, const ComputeBufferCounter* inBufferCounters,
	unsigned outBufferCount, const ComputeBufferID* outBuffers, const TextureID* outTextures, const TextureDimension* outTexDims, const UInt32* outBufferBindPoints, const ComputeBufferCounter* outBufferCounters)
{
	GLES_ASSERT(&m_Api, texCount == samplerCount, "We should have 1:1 matching lists of textures & samplers");

	for (unsigned i = 0; i < texCount; ++i)
	{
		if (textures[i].m_ID == 0)
			continue;

		GLES_ASSERT(&m_Api, texBindPoints[i] == (samplers[i] & 0xFFFF), "Extra check that the data structures match");

		GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(textures[i]);

		MemoryBarrierBeforeDraw(texInfo->imageWriteTime, gl::kBarrierTextureFetch);

		// TODO: Check if the texture is currently bound as RT (not sure if it matters tho)
		gles::SetTexture(m_State, texInfo->texture, texDims[i], texBindPoints[i] & 0xFFFF, (BuiltinSamplerState)(samplers[i] >> 16));
	}

	for (unsigned i = 0; i < inBufferCount; ++i)
	{
		SetComputeBuffer(inBuffers[i], inBufferBindPoints[i], inBufferCounters[i], true);
	}

	for (unsigned i = 0; i < outBufferCount; ++i)
	{
		if (outBufferBindPoints[i] & 0x80000000)
		{
			SetImageTexture(outTextures[i], outBufferBindPoints[i] & 0x7FFFFFFF);
		}
		else
		{
			SetComputeBuffer(outBuffers[i], outBufferBindPoints[i], outBufferCounters[i], false, true);
		}
	}
}


void GfxDeviceGLES::DispatchComputeProgram(ComputeProgramHandle cpHandle, unsigned threadGroupsX, unsigned threadGroupsY, unsigned threadGroupsZ)
{
	if (!cpHandle.IsValid())
		return;

	GLuint program = ((ComputeProgramGLES *)cpHandle.object)->program;
	gles::UseGLSLProgram(m_State, program);

	SetBarrierMask(kMemoryBarrierMaskDispatchCompute); // Ignore irrelevant barriers
	DoMemoryBarriers(); // Set required memory barriers before dispatch

	GLES_CALL(&m_Api, glDispatchCompute, threadGroupsX, threadGroupsY, threadGroupsZ);

	// TODO remove bindings that we've set so far.
}

void GfxDeviceGLES::DispatchComputeProgram(ComputeProgramHandle cpHandle, ComputeBufferID indirectBuffer, UInt32 argsOffset)
{
	if (!cpHandle.IsValid())
		return;
	
	ComputeBufferGLES *buf = GetComputeBufferGLES(indirectBuffer, m_ComputeBuffers);
	if (!buf)
		return;

	GLuint program = ((ComputeProgramGLES *)cpHandle.object)->program;
	gles::UseGLSLProgram(m_State, program);

	SetBarrierMask(kMemoryBarrierMaskDispatchCompute); // Ignore irrelevant barriers
	DoMemoryBarriers(); // Set required memory barriers before dispatch
	
	m_Api.BindDispatchIndirectBuffer(buf->m_Buffer->GetBuffer());
	GLES_CALL(&m_Api, glDispatchComputeIndirect, (GLintptr)argsOffset);

	// TODO remove bindings that we've set so far.
}

void GfxDeviceGLES::DrawNullGeometry(GfxPrimitiveType topology, int vertexCount, int instanceCount)
{
	GLES_ASSERT(&m_Api, GetGraphicsCaps().hasInstancing, "Instancing is not supported");

	BeforeDrawCall();

	m_Api.DrawArrays(topology, 0, vertexCount, instanceCount);
}

void GfxDeviceGLES::DrawNullGeometryIndirect(GfxPrimitiveType topology, ComputeBufferID bufferHandle, UInt32 bufferOffset)
{
	ComputeBufferGLES *buf = GetComputeBufferGLES(bufferHandle, m_ComputeBuffers);
	if (!buf)
		return;

	GLES_ASSERT(&m_Api, GetGraphicsCaps().gles.hasIndirectDraw, "Indirect draw is not supported");

	SetBarrierMask(kMemoryBarrierMaskIndirectDraw); // BeforeDrawCall() will handle barriers -> set mask here to include relevant barriers
	BeforeDrawCall();

	m_Api.BindDrawIndirectBuffer(buf->m_Buffer->GetBuffer());
	m_Api.DrawIndirect(topology, bufferOffset);
}

void GfxDeviceGLES::SetAntiAliasFlag(bool aa)
{
	if (!GetGraphicsCaps().gles.hasWireframe)
		return;

	if (aa)
	{
		m_Api.Enable(gl::kLineSmooth);
	}
	else
	{
		m_Api.Disable(gl::kLineSmooth);
	}
}

void GfxDeviceGLES::AddPendingMipGen(RenderSurfaceBase *rs)
{
	// Just add the surface to the list of pending surfaces
	m_PendingMipGens.push_back(rs);
}

void GfxDeviceGLES::ProcessPendingMipGens()
{
	// Generate mipmaps for all pending surfaces
	for (size_t i = 0; i < m_PendingMipGens.size(); i++)
	{
		GLuint textureName = ((GLESTexture*)TextureIdMap::QueryNativeTexture(m_PendingMipGens[i]->textureID))->texture;
		m_Api.GenerateMipmap(textureName, m_PendingMipGens[i]->dim);
	}
	m_PendingMipGens.clear();
}

void GfxDeviceGLES::CancelPendingMipGen(RenderSurfaceBase *rs)
{
	int i;
	for (i = 0; i < m_PendingMipGens.size(); i++)
	{
		if (m_PendingMipGens[i] == rs)
		{
			m_PendingMipGens.erase(m_PendingMipGens.begin() + i);
			i--; // decrement i before restarting the loop so we'll check all items; erase() moves things back by one.
			continue;
		}
	}
}

// -- verify state --

#if GFX_DEVICE_VERIFY_ENABLE
void GfxDeviceGLES::VerifyState()
{
}
#endif // GFX_DEVICE_VERIFY_ENABLE

#if UNITY_EDITOR && UNITY_WIN
GfxDeviceWindow* GfxDeviceGLES::CreateGfxWindow(HWND window, int width, int height, DepthBufferFormat depthFormat, int antiAlias)
{
	return new WindowGLES(window, width, height, depthFormat, antiAlias);
}
#endif//UNITY_EDITOR && UNITY_WIN

// -- pipeline setup --

const DeviceDepthStateGLES*		gles::CreateDepthState(DeviceStateGLES& state, GfxDepthState pipeState)
{
	return &*state.depthStateCache.insert(DeviceDepthStateGLES(pipeState)).first;
}
const DeviceStencilStateGLES*	gles::CreateStencilState(DeviceStateGLES& state, const GfxStencilState& pipeState)
{
	return &*state.stencilStateCache.insert(DeviceStencilStateGLES(pipeState)).first;
}
const DeviceBlendStateGLES*		gles::CreateBlendState(DeviceStateGLES& state, const GfxBlendState& pipeState)
{
	return &*state.blendStateCache.insert(DeviceBlendStateGLES(pipeState)).first;
}
const DeviceRasterStateGLES*	gles::CreateRasterState(DeviceStateGLES& state, const GfxRasterState& pipeState)
{
	return &*state.rasterStateCache.insert(DeviceRasterStateGLES(pipeState)).first;
}

namespace {
void SetDepthState(ApiGLES & api, DeviceStateGLES& state, const DeviceDepthStateGLES* newState)
{
	const DeviceDepthStateGLES* curState = state.depthState;
	if(curState == newState)
		return;

	state.depthState = newState;
	GLES_ASSERT(gGL, state.depthState, "Invalid depth state object");

	const CompareFunction newDepthFunc = (CompareFunction)newState->sourceState.depthFunc;
	const CompareFunction curDepthFunc = (CompareFunction)curState->sourceState.depthFunc;

	if(curDepthFunc != newDepthFunc)
	{
		if(newDepthFunc == kFuncDisabled)
		{
			api.Disable(gl::kDepthTest);
		}
		else
		{
			if(curDepthFunc == kFuncDisabled)
				api.Enable(gl::kDepthTest);
			GLES_CALL(&api, glDepthFunc, newState->glFunc);
		}
	}

	const bool write = newState->sourceState.depthWrite;
	if(write != curState->sourceState.depthWrite)
		GLES_CALL(&api, glDepthMask, write?GL_TRUE:GL_FALSE);
}

bool BlendingDisabled(const DeviceBlendStateGLES& blend)
{
	return blend.glSrcBlend == GL_ONE && blend.glDstBlend == GL_ZERO && blend.glSrcBlendAlpha == GL_ONE && blend.glDstBlendAlpha == GL_ZERO;
}

void ApplyColorMask(ApiGLES & api, const UInt32 colorMask)
{
	const bool colorWrite[] =
	{
		(colorMask & kColorWriteR) != 0, (colorMask & kColorWriteG) != 0,
		(colorMask & kColorWriteB) != 0, (colorMask & kColorWriteA) != 0
	};
	GLES_CALL(&api, glColorMask, colorWrite[0], colorWrite[1], colorWrite[2], colorWrite[3]);
}

bool BlendFuncEquals(const DeviceBlendStateGLES& a, const DeviceBlendStateGLES& b)
{
	return	a.glSrcBlend == b.glSrcBlend &&
			a.glDstBlend == b.glDstBlend &&
			a.glSrcBlendAlpha == b.glSrcBlendAlpha &&
			a.glDstBlendAlpha == b.glDstBlendAlpha;
}

bool BlendOpEquals(const DeviceBlendStateGLES& a, const DeviceBlendStateGLES& b)
{
	return	a.glBlendOp == b.glBlendOp && a.glBlendOpAlpha == b.glBlendOpAlpha;
}

void SetBlendState(ApiGLES & api, DeviceStateGLES& state, const DeviceBlendStateGLES* newState)
{
	const DeviceBlendStateGLES* curState = state.blendState;
	if (curState == newState)
		return;

	state.blendState = newState;
	GLES_ASSERT(gGL, state.blendState, "Invalid blend state");

	if(curState != newState)
	{
		const UInt32 newColorMask = newState->sourceState.renderTargetWriteMask;
		if(curState->sourceState.renderTargetWriteMask != newColorMask)
			ApplyColorMask(api, newColorMask);

		const bool curBlendDisabled = BlendingDisabled(*curState);
		if (BlendingDisabled(*newState))
		{
			if (!curBlendDisabled)
				api.Disable(gl::kBlend);

			// Don't care about glBlendFunc and glBlendEquation when blending is disabled.
		}
		else
		{
			if (curBlendDisabled)
				api.Enable(gl::kBlend);
			
			// Force update of glBlendFunc and glBlendEquation if blending was disabled ('curBlendDisabled').
			if (curBlendDisabled || !BlendFuncEquals(*curState, *newState))
				GLES_CALL(&api, glBlendFuncSeparate, newState->glSrcBlend, newState->glDstBlend, newState->glSrcBlendAlpha, newState->glDstBlendAlpha);

			if (curBlendDisabled || !BlendOpEquals(*curState, *newState))
			{
				enum equType { equSingle, equSeparate, equUnsupported };
				equType supportedBlendEqu = equSeparate;

				// Check advanced blend
				if (newState->blendFlags & kBlendFlagAdvanced)
				{
					if (GetGraphicsCaps().hasBlendAdvanced)
						supportedBlendEqu = equSingle; //Advanced blends support only single equations call
					else
						supportedBlendEqu = equUnsupported;
				}

				if (!GetGraphicsCaps().hasBlendMinMax && (newState->blendFlags & kBlendFlagMinMax))
					supportedBlendEqu = equUnsupported;

				if (supportedBlendEqu == equSeparate)
					GLES_CALL(&api, glBlendEquationSeparate, newState->glBlendOp, newState->glBlendOpAlpha);
				else if (supportedBlendEqu == equSingle)
					GLES_CALL(&api, glBlendEquation, newState->glBlendOp);

				// intentional fallthrough on equUnsupported -> unsuppported state
			}
		}

		if (newState->sourceState.alphaToMask)
			api.Enable(gl::kSampleAlphaToCoverage);
		else
			api.Disable(gl::kSampleAlphaToCoverage);
	}
}

void SetRasterState(ApiGLES & api, DeviceStateGLES& state, const DeviceRasterStateGLES* newState)
{
	const DeviceRasterStateGLES* curState = state.rasterState;
	if(curState == newState)
		return;

	state.rasterState = newState;
	GLES_ASSERT(gGL, state.rasterState, "Invalid raster state");

	CullMode cull = newState->sourceState.cullMode;
	if(cull != curState->sourceState.cullMode)
		api.SetCullMode(cull);

	float zFactor = newState->sourceState.slopeScaledDepthBias;
	float zUnits  = newState->sourceState.depthBias;
	if(zFactor != curState->sourceState.slopeScaledDepthBias || zUnits != curState->sourceState.depthBias)
	{
	#if UNITY_ANDROID
		// Compensate for PolygonOffset bug on old Mali and Pvr GPUs
		if (GetGraphicsCaps().gles.haspolygonOffsetBug)
		{
			zFactor *= 16.0F;
		}
	#endif

		GLES_CALL(&api, glPolygonOffset, zFactor, zUnits);
		if(zFactor || zUnits)
			api.Enable(gl::kPolygonOffsetFill);
		else
			api.Disable(gl::kPolygonOffsetFill);
	}
}
}//namespace

#define UPDATE_PIPELINE_STATE_IMPL(StateType, SrcStateType, state, pipeState, createFunc, member, val)	\
do{																										\
	const StateType* srcState = pipeState ? pipeState : state.pipeState;								\
	GLES_ASSERT(gGL, srcState != 0 && state.pipeState != 0, "Invalid pipeline state");					\
																										\
	SrcStateType srcData = srcState->sourceState;														\
	if(srcData.member == val)																			\
		return srcState;																				\
																										\
	srcData.member = val;																				\
	return createFunc(state, srcData);																	\
} while(0)


const DeviceDepthStateGLES*		gles::UpdateDepthTest(DeviceStateGLES& state, const DeviceDepthStateGLES* depthState, bool enable)
{
	UPDATE_PIPELINE_STATE_IMPL(DeviceDepthStateGLES, GfxDepthState, state, depthState, gles::CreateDepthState, depthWrite, enable);
}
const DeviceStencilStateGLES*	gles::UpdateStencilMask(DeviceStateGLES& state, const DeviceStencilStateGLES* stencilState, UInt8 stencilMask)
{
	UPDATE_PIPELINE_STATE_IMPL(DeviceStencilStateGLES, GfxStencilState, state, stencilState, gles::CreateStencilState, writeMask, stencilMask);
}
const DeviceBlendStateGLES*		gles::UpdateColorMask(DeviceStateGLES& state, const DeviceBlendStateGLES* blendState, UInt32 colorMask)
{
	UPDATE_PIPELINE_STATE_IMPL(DeviceBlendStateGLES, GfxBlendState, state, blendState, gles::CreateBlendState, renderTargetWriteMask, colorMask);
}

#undef UPDATE_PIPELINE_STATE_IMPL


const DeviceRasterStateGLES* gles::AddDepthBias(DeviceStateGLES& state, const DeviceRasterStateGLES* rasterState, float depthBias, float slopeDepthBias)
{
	GfxRasterState srcData = rasterState ? rasterState->sourceState : state.rasterState->sourceState;

	srcData.depthBias				+= depthBias;
	srcData.slopeScaledDepthBias	+= slopeDepthBias;

	return CreateRasterState(state, srcData);
}

const DeviceRasterStateGLES* gles::AddForceCullMode(DeviceStateGLES& state, const DeviceRasterStateGLES* rasterState, CullMode cull)
{
	GfxRasterState srcData = rasterState ? rasterState->sourceState : state.rasterState->sourceState;

	srcData.cullMode = cull;
	
	return CreateRasterState(state, srcData);
}

void gles::ClearCurrentFramebuffer(ApiGLES * api, bool clearColor, bool clearDepth, bool clearStencil, const ColorRGBAf& color, float depth, int stencil)
{
	GLES_ASSERT(gGL, api->debug.FramebufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

	DeviceStateGLES& state = *g_DeviceStateGLES;
	GLbitfield flags = 0;

	if (clearColor)
	{
		::SetBlendState(*api, state, gles::UpdateColorMask(state, 0, 15));
		flags |= GL_COLOR_BUFFER_BIT | (g_GraphicsCapsGLES->hasNVCSAA ? GL_COVERAGE_BUFFER_BIT_NV : 0);
	}

	if (clearDepth)
	{
		::SetDepthState(*api, state, gles::UpdateDepthTest(state, 0, true));
		flags |= GL_DEPTH_BUFFER_BIT;
	}

	if (clearStencil)
	{
		GetRealGfxDevice().SetStencilState(gles::UpdateStencilMask(state, 0, 0xff), state.stencilRefValue);
		flags |= GL_STENCIL_BUFFER_BIT;
	}

	api->Clear(flags, color, false, depth, stencil);
}

