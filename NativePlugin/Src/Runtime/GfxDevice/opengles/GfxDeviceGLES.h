#pragma once

#include "Runtime/GfxDevice/GfxDeviceConfigure.h"
#include "ApiGLES.h"
#include "DeviceStateGLES.h"
#include "VertexDeclarationGLES.h"
#include "Runtime/GfxDevice/GfxDevice.h"

#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
#include "DisplayManagerGLES.h"
#endif

class ComputeBufferGLES;
class GfxFramebufferGLES;
class GfxContextGLES;
class ApiGLES;

class GfxDeviceGLES : public GfxThreadableDevice
{
public:
					GfxDeviceGLES();
	virtual			~GfxDeviceGLES();

	// It needs to be called before any use of an GfxDeviceGLES instance
	// It should be in GfxDeviceGLES constructor but we create GfxDeviceGLES with UNITY_NEW_AS_ROOT which doesn't allow arguments
	GFX_API bool	Init(GfxDeviceLevelGL deviceLevel);
	GFX_API void	OnDeviceCreated(bool callingFromRenderThread);

	GFX_API void	BeforePluginRender();
	GFX_API void	AfterPluginRender();

	GFX_API void	BeginFrame();
	GFX_API void	EndFrame();
	GFX_API void	PresentFrame();

	GFX_API GfxBuffer* CreateIndexBuffer();
	GFX_API GfxBuffer* CreateVertexBuffer();
	GFX_API void	UpdateBuffer(GfxBuffer* buffer, GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data, UInt32 flags);
	GFX_API void*	BeginBufferWrite(GfxBuffer* buffer, size_t offset = 0, size_t size = 0);
	GFX_API void	EndBufferWrite(GfxBuffer* buffer, size_t bytesWritten);
	GFX_API void	DeleteBuffer(GfxBuffer* buffer);

	GFX_API VertexDeclaration* GetVertexDeclaration(const VertexChannelsInfo& declKey);

	GFX_API void	BeforeDrawCall();

	GFX_API void	DrawBuffers(GfxBuffer* indexBuf,
		const VertexStreamSource* vertexStreams, int vertexStreamCount,
		const DrawBuffersRange* drawRanges, int drawRangeCount,
		VertexDeclaration* vertexDecl, const ChannelAssigns& channels);

	GFX_API void	SetSRGBWrite(const bool);
	GFX_API bool	GetSRGBWrite();

	GFX_API void	SetUserBackfaceMode(bool enable);
	GFX_API void	SetForceCullMode (CullMode mode);
	GFX_API void	SetWireframe(bool wire);
	GFX_API bool	GetWireframe() const;
	GFX_API void	SetInvertProjectionMatrix(bool enable);

	GFX_API void	SetWorldMatrix(const Matrix4x4f& matrix);
	GFX_API void	SetViewMatrix(const Matrix4x4f& matrix);
	GFX_API void	SetProjectionMatrix(const Matrix4x4f& matrix);

	GFX_API void	SetBackfaceMode(bool backface);
	GFX_API void	SetViewport(const RectInt& rect);
	GFX_API RectInt	GetViewport() const;

	GFX_API void	DisableScissor();
	GFX_API bool	IsScissorEnabled() const;
	GFX_API void	SetScissorRect(const RectInt& rect);
	GFX_API RectInt	GetScissorRect() const;

	GFX_API void	SetTextures (ShaderType shaderType, int count, const GfxTextureParam* textures);
	GFX_API void	SetTextureParams(TextureID texture, TextureDimension texDim, TextureFilterMode filter, TextureWrapMode wrap, int anisoLevel, float mipBias, bool hasMipMap, TextureColorSpace colorSpace, ShadowSamplingMode shadowSamplingMode);

	GFX_API void	SetShadersThreadable(GpuProgram* programs[kShaderTypeCount], const GpuProgramParameters* params[kShaderTypeCount], UInt8 const * const paramsBuffer[kShaderTypeCount]);

	GFX_API bool	IsShaderActive(ShaderType type) const;
	GFX_API void	DestroySubProgram(ShaderLab::SubProgram* subprogram);
	GFX_API void	DestroyGpuProgram(GpuProgram const * const program);

	GFX_API void	SetShaderPropertiesCopied(const ShaderPropertySheet& properties);

	GFX_API void				DiscardContents(RenderSurfaceHandle& rs);
	GFX_API void				SetRenderTargets(const GfxRenderTargetSetup& rt);
	GFX_API void				ResolveColorSurface(RenderSurfaceHandle srcHandle, RenderSurfaceHandle dstHandle);
	GFX_API void				ResolveDepthIntoTexture(RenderSurfaceHandle colorHandle, RenderSurfaceHandle depthHandle);
	GFX_API RenderSurfaceHandle GetActiveRenderColorSurface(int index) const;
	GFX_API RenderSurfaceHandle GetActiveRenderDepthSurface() const;
	GFX_API int					GetCurrentTargetAA() const;
	GFX_API int GetActiveRenderTargetCount () const;

	GFX_API bool				CaptureScreenshot(int left, int bottom, int width, int height, UInt8* rgba32);
	GFX_API bool				ReadbackImage(ImageReference& image, int left, int bottom, int width, int height, int destX, int destY);
	GFX_API void				GrabIntoRenderTexture(RenderSurfaceHandle rs, RenderSurfaceHandle rd, int x, int y, int width, int height);

	GFX_API void RegisterNativeTexture(TextureID texture, intptr_t nativeTex, TextureDimension dim);
	GFX_API void UnregisterNativeTexture(TextureID texture);

	GFX_API void UploadTexture2D(TextureID texture, TextureDimension dimension, const UInt8* srcData, int srcSize, int width, int height, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureUsageMode usageMode, TextureColorSpace colorSpace);
	GFX_API void UploadTextureSubData2D(TextureID texture, const UInt8* srcData, int srcSize, int mipLevel, int x, int y, int width, int height, TextureFormat format, TextureColorSpace colorSpace);
	GFX_API void UploadTextureCube(TextureID texture, const UInt8* srcData, int srcSize, int faceDataSize, int size, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureColorSpace colorSpace);
	GFX_API void UploadTexture3D(TextureID texture, const UInt8* srcData, int srcSize, int width, int height, int depth, TextureFormat format, int mipCount, UInt32 uploadFlags);
	GFX_API void DeleteTexture(TextureID texture);

	GFX_API void UploadTexture2DArray(TextureID texture, const UInt8* srcData, size_t elementSize, int width, int height, int depth, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureColorSpace colorSpace);

	GFX_API SparseTextureInfo CreateSparseTexture(TextureID texture, int width, int height, TextureFormat format, int mipCount, TextureColorSpace colorSpace);
	GFX_API void UploadSparseTextureTile(TextureID texture, int tileX, int tileY, int mip, const UInt8* srcData, int srcSize, int srcPitch);

	GFX_API void CopyTexture(TextureID src, TextureID dst);
	GFX_API void CopyTexture(TextureID src, int srcElement, int srcMip, int srcMipCount, TextureID dst, int dstElement, int dstMip, int dstMipCount);
	GFX_API void CopyTexture(TextureID src, int srcElement, int srcMip, int srcMipCount, int srcX, int srcY, int srcWidth, int srcHeight, TextureID dst, int dstElement, int dstMip, int dstMipCount, int dstX, int dstY);

	GFX_API GfxDeviceLevelGL GetDeviceLevel() const;

	GFX_API PresentMode	GetPresentMode();

	GFX_API bool	IsValidState();
	GFX_API bool	HandleInvalidState();
	GFX_API void	InvalidateState();


	GFX_API void	Flush();
	GFX_API void	FinishRendering();

	GFX_API void	ReloadResources();

	GFX_API bool ActivateDisplay(const UInt32 /*displayId*/);
	GFX_API bool SetDisplayTarget(const UInt32 /*displayId*/);

	GFX_API void SetSurfaceFlags(RenderSurfaceHandle surf, UInt32 flags, UInt32 keepFlags);
	GFX_API void SetStereoTarget(StereoscopicEye eye);

#if ENABLE_PROFILER
	GFX_API void				BeginProfileEvent(const char* name);
	GFX_API void				EndProfileEvent();

	GFX_API GfxTimerQuery*		CreateTimerQuery();
	GFX_API void				DeleteTimerQuery(GfxTimerQuery* query);
	GFX_API void				BeginTimerQueries();
	GFX_API void				EndTimerQueries();
#endif

	GFX_API void	SetTextureName(TextureID tex, const char* name);
	GFX_API void	SetRenderSurfaceName(RenderSurfaceBase* rs, const char* name);
	GFX_API void	SetBufferName(GfxBuffer* buffer, const char* name);
	GFX_API void	SetGpuProgramName(GpuProgram* prog, const char* name);

public:
	GFX_API void RenderTargetBarrier();

	GFX_API const DeviceBlendState*		CreateBlendState(const GfxBlendState& state);
	GFX_API const DeviceDepthState*		CreateDepthState(const GfxDepthState& state);
	GFX_API const DeviceStencilState*	CreateStencilState(const GfxStencilState& state);
	GFX_API const DeviceRasterState*	CreateRasterState(const GfxRasterState& state);

	GFX_API void						SetRasterState(const DeviceRasterState* state);
	GFX_API void						SetStencilState(const DeviceStencilState* state, int stencilRef);
	GFX_API void						SetBlendState(const DeviceBlendState* state);
	GFX_API void						SetDepthState(const DeviceDepthState* state);

	GFX_API void	Clear(UInt32 clearFlags, const ColorRGBAf& color, float depth, UInt32 stencil);

	GFX_API GPUSkinPoseBuffer* CreateGPUSkinPoseBuffer();
	GFX_API void	DeleteGPUSkinPoseBuffer(GPUSkinPoseBuffer* poseBuffer);
	GFX_API void	UpdateSkinPoseBuffer(GPUSkinPoseBuffer* poseBuffer, const Matrix4x4f* boneMatrices, int boneCount);

	GFX_API void	SkinOnGPU(const VertexStreamSource& sourceMesh, GfxBuffer* skinBuffer, GPUSkinPoseBuffer* poseBuffer, GfxBuffer* destBuffer, int vertexCount, int bonesPerVertex, UInt32 channelMask, bool lastThisFrame);

	GFX_API void	SetActiveContext(void* context);

	GFX_API void	AcquireThreadOwnership();
	GFX_API void	ReleaseThreadOwnership();

	GFX_API void	UpdateConstantBuffer(int id, size_t size, const void* data);

	GFX_API RenderTextureFormat	GetDefaultRTFormat() const;
	GFX_API RenderTextureFormat	GetDefaultHDRRTFormat() const;

	GFX_API void* GetNativeTexturePointer(TextureID id);

	void MemoryBarrierBeforeDraw(BarrierTime previousWriteTime, gl::MemoryBarrierType type);
	void MemoryBarrierImmediate(BarrierTime previousWriteTime, gl::MemoryBarrierType type);
	void SetBarrierMask(GLbitfield mask);
	void DoMemoryBarriers();
	
	GFX_API void SetComputeBufferCounterValue(ComputeBufferID bufferHandle, UInt32 value);
	void SetComputeBuffer(ComputeBufferID bufferHandle, int index, ComputeBufferCounter counter, bool recordRender = false, bool recordUpdate = false);
	GFX_API void SetComputeBufferData(ComputeBufferID bufferHandle, const void* data, size_t size);
	GFX_API void GetComputeBufferData(ComputeBufferID bufferHandle, void* dest, size_t destSize);
	GFX_API void CopyComputeBufferCount(ComputeBufferID srcBuffer, ComputeBufferID dstBuffer, UInt32 dstOffset);

	void SetImageTexture(TextureID tid, int index);
	GFX_API void SetRandomWriteTargetTexture(int index, TextureID tid);
	GFX_API void SetRandomWriteTargetBuffer(int index, ComputeBufferID bufferHandle);
	GFX_API void ClearRandomWriteTargets();
	
	GFX_API bool HasActiveRandomWriteTarget () const;

	GFX_API ComputeProgramHandle CreateComputeProgram(const UInt8* code, size_t codeSize);
	GFX_API void DestroyComputeProgram(ComputeProgramHandle& cpHandle);
	GFX_API void ResolveComputeProgramResources(ComputeProgramHandle cpHandle, ComputeShaderKernel& kernel, std::vector<ComputeShaderCB>& constantBuffers, std::vector<ComputeShaderParam>& uniforms, bool preResolved);
	GFX_API void CreateComputeConstantBuffers(unsigned count, const UInt32* sizes, ConstantBufferHandle* outCBs);
	GFX_API void DestroyComputeConstantBuffers(unsigned count, ConstantBufferHandle* cbs);
	GFX_API void CreateComputeBuffer(ComputeBufferID id, size_t count, size_t stride, UInt32 flags);
	GFX_API void DestroyComputeBuffer(ComputeBufferID handle);
	GFX_API void SetComputeUniform(ComputeProgramHandle cpHandle, ComputeShaderParam& uniform, size_t byteCount, const void* data);
	GFX_API void UpdateComputeConstantBuffers(unsigned count, ConstantBufferHandle* cbs, UInt32 cbDirty, size_t dataSize, const UInt8* data, const UInt32* cbSizes, const UInt32* cbOffsets, const int* bindPoints);
	GFX_API void UpdateComputeResources(
		unsigned texCount, const TextureID* textures, const TextureDimension* texDims, const int* texBindPoints,
		unsigned samplerCount, const unsigned* samplers,
		unsigned inBufferCount, const ComputeBufferID* inBuffers, const int* inBufferBindPoints, const ComputeBufferCounter* inBufferCounters,
		unsigned outBufferCount, const ComputeBufferID* outBuffers, const TextureID* outTextures, const TextureDimension* outTexDims, const UInt32* outBufferBindPoints, const ComputeBufferCounter* outBufferCounters);
	GFX_API void DispatchComputeProgram(ComputeProgramHandle cpHandle, unsigned threadGroupsX, unsigned threadGroupsY, unsigned threadGroupsZ);
	GFX_API void DispatchComputeProgram(ComputeProgramHandle cpHandle, ComputeBufferID indirectBuffer, UInt32 argsOffset);

	GFX_API void DrawNullGeometry(GfxPrimitiveType topology, int vertexCount, int instanceCount);
	GFX_API void DrawNullGeometryIndirect(GfxPrimitiveType topology, ComputeBufferID bufferHandle, UInt32 bufferOffset);
	GFX_API void SetAntiAliasFlag(bool aa);

	// This will copy whole platform render surface descriptor (and not just store argument pointer)
	GFX_API void SetBackBufferColorDepthSurface(RenderSurfaceBase* color, RenderSurfaceBase* depth);

	// Temporary hack, really m_State should be a member of GfxContextGLES
	DeviceStateGLES& State() {return m_State;}

	GfxContextGLES& Context() { return *m_Context;  }

	GfxFramebufferGLES& GetFramebuffer();

#if GFX_DEVICE_VERIFY_ENABLE
	GFX_API void	VerifyState();
#endif

#if UNITY_EDITOR && UNITY_WIN
	GFX_API GfxDeviceWindow* CreateGfxWindow(HWND window, int width, int height, DepthBufferFormat depthFormat, int antiAlias);
#endif//UNITY_EDITOR && UNITY_WIN

	// Machinery for generating mipmaps for rendersurfaces. Previously we used to call glGenMipmaps in GfxFramebuffer::Prepare() for the previous
	// rendersurfaces as needed, but that doesn't work when we switch GL context (and therefore do ApiGLES::Invalidate) after rendering to the
	// texture but before rendering anything else to another rendertarget (so Prepare never gets called for that one).
	// Therefore we keep a list of surfaces that need mipmap generation, and call ProcessPendingMipGens() at both Prepare and right after
	// switching GL context. We'll also have to handle the case where the render texture gets deleted while the mipmaps are still in the pending list (that's what CancelPendingMipGen is for).
	void ProcessPendingMipGens(); // Generate mipmaps for all pending render textures
	void AddPendingMipGen(RenderSurfaceBase *rs); // Register a render texture for pending automatic mipmap generation
	void CancelPendingMipGen(RenderSurfaceBase *rs); // Remove a rendertexture from the list of pending mipmap generations. Called if the texture is deleted.

protected:

	void ApplyBackfaceMode();
	void UpdateSRGBWrite();

	GFX_API DynamicVBO*	CreateDynamicVBO();

	GFX_API size_t	RenderSurfaceStructMemorySize(bool colorSurface);
	GFX_API void	AliasRenderSurfacePlatform(RenderSurfaceBase* rs, TextureID origTexID);
	GFX_API bool	CreateColorRenderSurfacePlatform(RenderSurfaceBase* rs, RenderTextureFormat format);
	GFX_API bool	CreateDepthRenderSurfacePlatform(RenderSurfaceBase* rs, DepthBufferFormat format);
	GFX_API void	DestroyRenderSurfacePlatform(RenderSurfaceBase* rs);

	GfxContextGLES*				m_Context;
	ApiGLES						m_Api;
	DeviceStateGLES				m_State;
	// used to init common device state struct
	void						InitCommonState(DeviceStateGLES& state);

	VertexDeclarationCacheGLES	vertDeclCache;

	//TODO: These are totally artificial numbers. Query real limits and decide based on those.
	enum { 
		kMaxAtomicCounters = 256
	};
	DataBufferGLES* m_AtomicCounterBuffer; // One big atomic counter buffer to back all the counters a shader needs
	ComputeBufferGLES* m_AtomicCounterSlots[kMaxAtomicCounters]; // Which buffer's counter is at which slot currently (to avoid unnecessary copies)

	std::map<ComputeBufferID, ComputeBufferGLES *> m_ComputeBuffers; //TODO: use some faster map implementation
	std::map<ComputeBufferID, DataBufferGLES *> m_ComputeConstantBuffers; //TODO: use some faster map implementation

	dynamic_array<RenderSurfaceBase *> m_PendingMipGens; // List of rendersurfaces that need automatic mipmap generation.
#if SUPPORT_MULTIPLE_DISPLAYS && UNITY_STANDALONE
	DisplayManagerGLES m_DisplayManager;
#endif
};


// a lot of api are easier implemented in c-like way
// these are functions that otherwise would be GfxDevice members

namespace gles
{
	// pipeline setup

	const DeviceDepthStateGLES*		CreateDepthState(DeviceStateGLES& state, GfxDepthState pipeState);
	const DeviceStencilStateGLES*	CreateStencilState(DeviceStateGLES& state, const GfxStencilState& pipeState);
	const DeviceBlendStateGLES*		CreateBlendState(DeviceStateGLES& state, const GfxBlendState& pipeState);
	const DeviceRasterStateGLES*	CreateRasterState(DeviceStateGLES& state, const GfxRasterState& pipeState);

	// updating pipeline states:
	// if provided state is null, current pipeline state will be tweaked
	// NB: pipeline states are immutable, so nothing is modified, rather another "cloned" state is returned

	const DeviceDepthStateGLES*		UpdateDepthTest(DeviceStateGLES& state, const DeviceDepthStateGLES* depthState, bool enable);
	const DeviceStencilStateGLES*	UpdateStencilMask(DeviceStateGLES& state, const DeviceStencilStateGLES* stencilState, UInt8 stencilMask);
	const DeviceBlendStateGLES*		UpdateColorMask(DeviceStateGLES& state, const DeviceBlendStateGLES* blendState, UInt32 colorMask);
	const DeviceRasterStateGLES*	AddDepthBias(DeviceStateGLES& state, const DeviceRasterStateGLES* rasterState, float depthBias, float slopeDepthBias);
	const DeviceRasterStateGLES*	AddForceCullMode(DeviceStateGLES& state, const DeviceRasterStateGLES* rasterState, CullMode cull);

	// low level clear implementation
	// we will use it different places so we put them here
	void ClearCurrentFramebuffer(ApiGLES * api, bool clearColor, bool clearDepth, bool clearStencil, const ColorRGBAf& color, float depth, int stencil);
}
