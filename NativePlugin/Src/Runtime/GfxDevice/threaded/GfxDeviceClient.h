#pragma once
#if ENABLE_MULTITHREADED_CODE

#include "Runtime/GfxDevice/GfxDevice.h"
#include "Runtime/GfxDevice/threaded/ThreadedDisplayList.h"
#include "Runtime/GfxDevice/threaded/ThreadedDeviceStates.h"
#include "Runtime/GfxDevice/threaded/ThreadedVertexDeclaration.h"
#include "Runtime/Shaders/GlobalConstantBuffers.h"
#include "Runtime/Utilities/dynamic_array.h"

class ThreadedStreamBuffer;
class GfxDeviceWorker;
class GfxDeviceWindow;
class ThreadedWindow;
class ThreadedDynamicVBO;
struct DisplayListContext;

enum
{
	kClientDeviceThreaded = 1 << 0,
	kClientDeviceForceRef = 1 << 1,
	kClientDeviceUseRealDevice = 1 << 2,
	kClientDeviceClientProcess = 1 << 3,
	kClientDeviceWorkerProcess = 1 << 4,
	kClientDeviceThreadedClientProcess  = 1 << 5,
};

GfxDevice* CreateClientGfxDevice(GfxDeviceRenderer renderer, UInt32 flags);


class GfxDeviceClient : public GfxDevice
{
public:
	GfxDeviceClient(bool threaded, UInt32 flags);
	GFX_API ~GfxDeviceClient();

	GFX_API void	InvalidateState();
	GFX_API void	CopyContextDataFrom(const GfxDevice* device);

	GFX_API void	BeforePluginRender();
	GFX_API void	AfterPluginRender();

#if GFX_DEVICE_VERIFY_ENABLE
	GFX_API void	VerifyState();
#endif

	GFX_API void	SetMaxBufferedFrames (int bufferSize);

	GFX_API void	Clear (UInt32 clearFlags, const ColorRGBAf& color, float depth, UInt32 stencil);
	GFX_API void	SetUserBackfaceMode( bool enable );
	GFX_API void SetForceCullMode (CullMode mode);
	GFX_API void SetWireframe(bool wire);
	GFX_API bool GetWireframe() const;

	GFX_API void	SetInvertProjectionMatrix( bool enable );

	GFX_API GPUSkinPoseBuffer* CreateGPUSkinPoseBuffer();
	GFX_API void	DeleteGPUSkinPoseBuffer(GPUSkinPoseBuffer* poseBuffer);
	GFX_API void	UpdateSkinPoseBuffer(GPUSkinPoseBuffer* poseBuffer, const Matrix4x4f* boneMatrices, int boneCount);

	GFX_API void	SkinOnGPU(const VertexStreamSource& sourceMesh, GfxBuffer* skinBuffer, GPUSkinPoseBuffer* poseBuffer, GfxBuffer* destBuffer, int vertexCount, int bonesPerVertex, UInt32 channelMask, bool lastThisFrame);

	GFX_API void RenderTargetBarrier();

	GFX_API const DeviceBlendState* CreateBlendState(const GfxBlendState& state);
	GFX_API const DeviceDepthState* CreateDepthState(const GfxDepthState& state);
	GFX_API const DeviceStencilState* CreateStencilState(const GfxStencilState& state);
	GFX_API const DeviceRasterState* CreateRasterState(const GfxRasterState& state);

	GFX_API void	SetBlendState(const DeviceBlendState* state);
	GFX_API void	SetDepthState(const DeviceDepthState* state);
	GFX_API void	SetStencilState(const DeviceStencilState* state, int stencilRef);
	GFX_API void	SetStencilRefWhenStencilWasSkipped (int stencilRef);

	GFX_API void	SetRasterState(const DeviceRasterState* state);
	GFX_API void	SetSRGBWrite (const bool);
	GFX_API bool	GetSRGBWrite ();

	GFX_API void	SetWorldMatrix( const Matrix4x4f& matrix );
	GFX_API void	SetViewMatrix( const Matrix4x4f& matrix );
	GFX_API void	SetProjectionMatrix(const Matrix4x4f& matrix);
	GFX_API void	UpdateViewProjectionMatrix();

#if GFX_SUPPORTS_SINGLE_PASS_STEREO
	GFX_API void	SetStereoMatrix(MonoOrStereoscopicEye monoOrStereoEye, BuiltinShaderMatrixParam param, const Matrix4x4f& matrix);
	GFX_API void	SetStereoViewport(StereoscopicEye eye,  const RectInt& rect);
	GFX_API RectInt	GetStereoViewport(StereoscopicEye eye) const;
	GFX_API void	SetStereoScissorRects(const RectInt rects[kStereoscopicEyeCount]);
	GFX_API void	SetStereoConstantBuffers (int leftID, int rightID, int monoscopicID, size_t size);
	GFX_API void	SetSinglePassStereoEyeMask(TargetEyeMask eyeMask);
	GFX_API void	SaveStereoConstants();
	GFX_API void	RestoreStereoConstants();
#endif

	GFX_API void	SetBackfaceMode( bool backface );

	GFX_API void	SetViewport( const RectInt& rect );
	GFX_API RectInt	GetViewport() const;


	GFX_API void	SetScissorRect( const RectInt& rect );
	GFX_API void	DisableScissor();
	GFX_API bool	IsScissorEnabled() const;
	GFX_API RectInt	GetScissorRect() const;

	GFX_API void	SetTextures (ShaderType shaderType, int count, const GfxTextureParam* textures);
	GFX_API void	SetTextureParams( TextureID texture, TextureDimension texDim, TextureFilterMode filter, TextureWrapMode wrap, int anisoLevel, float mipBias, bool hasMipMap, TextureColorSpace colorSpace, ShadowSamplingMode shadowSamplingMode );

	GFX_API void	SetShaderPropertiesCopied(const ShaderPropertySheet& properties);
	GFX_API void	SetShaderPropertiesShared(const ShaderPropertySheet& properties);

	GFX_API GpuProgram* CreateGpuProgram( ShaderGpuProgramType shaderProgramType, const dynamic_array<UInt8>& source, CreateGpuProgramOutput& output );
	GFX_API void	DestroyGpuProgram (GpuProgram const * const program);
	GFX_API void	SetShadersMainThread( const SubPrograms& programs, const ShaderPropertySheet* localProps, const ShaderPropertySheet* globalProps );
	GFX_API bool	IsShaderActive( ShaderType type ) const;
	GFX_API void	DestroySubProgram( ShaderLab::SubProgram* subprogram );
	GFX_API void	UpdateConstantBuffer(int id, size_t size, const void* data);

	GFX_API GfxBuffer* CreateIndexBuffer();
	GFX_API GfxBuffer* CreateVertexBuffer();
	GFX_API void	UpdateBuffer(GfxBuffer* buffer, GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data, UInt32 flags);
	GFX_API void*	BeginBufferWrite(GfxBuffer* buffer, size_t offset = 0, size_t size = 0);
	GFX_API void	EndBufferWrite(GfxBuffer* buffer, size_t bytesWritten);
	GFX_API void	DeleteBuffer(GfxBuffer* buffer);

	GFX_API VertexDeclaration* GetVertexDeclaration( const VertexChannelsInfo& declKey );

	GFX_API void	DrawBuffers(GfxBuffer* indexBuf,
						const VertexStreamSource* vertexStreams, int vertexStreamCount,
						const DrawBuffersRange* drawRanges, int drawRangeCount,
						VertexDeclaration* vertexDecl, const ChannelAssigns& channels );

protected:
	GFX_API void	ScheduleGeometryJobsInternal( GeometryJobFunc* geometryJob, const GeometryJobInstruction* geometryJobDatas, UInt32 geometryJobCount);
	GFX_API void	ScheduleDynamicVBOGeometryJobsInternal( GeometryJobFunc* geometryJob, GeometryJobInstruction* geometryJobDatas, UInt32 geometryJobCount, GfxPrimitiveType primType, DynamicVBOChunkHandle* outChunk );

public:
	GFX_API void	PutGeometryJobFence(GeometryJobFence skinningInstance);
	GFX_API void	EndGeometryJobFrame();


#if GFX_ENABLE_DRAW_CALL_BATCHING
#if ENABLE_PROFILER
	GFX_API	void	AddBatchStats( GfxBatchStatsType type, int batchedTris, int batchedVerts, int batchedCalls, TimeFormat batchedDt );
#endif
	GFX_API	void	BeginDynamicBatching( const ChannelAssigns& shaderChannels, UInt32 channelsMask, UInt32 stride, VertexDeclaration* vertexDecl, size_t maxVertices, size_t maxIndices, GfxPrimitiveType topology );
	GFX_API	void	DynamicBatchMesh( const Matrix4x4f& matrix, const VertexBufferData& vertices, UInt32 firstVertex, UInt32 vertexCount, const UInt16* indices, UInt32 indexCount, GfxTransformVerticesFlags transformFlags );
	GFX_API	void	EndDynamicBatching( TransformType transformType );
#endif

	GFX_API void	AddSetPassStat();

	GFX_API	void	ReleaseSharedMeshData( SharedMeshData* data );
	GFX_API	void	ReleaseSharedTextureData( SharedTextureData* data );
	GFX_API	void	ReleaseAsyncCommandHeader( GfxDeviceAsyncCommand::Arg* header );

	// Platform specific API extensions are kept in the
	// PlatformDependent folder and implemented there.
	#define GFX_EXT_POSTFIX
	#include "Runtime/GfxDevice/PlatformSpecificGfxDeviceExt.h"
	#undef  GFX_EXT_POSTFIX

	GFX_API void DiscardContents (RenderSurfaceHandle& rs);
	GFX_API void IgnoreNextUnresolveOnCurrentRenderTarget();
	GFX_API void IgnoreNextUnresolveOnRS(RenderSurfaceHandle rs);
	GFX_API void SetRenderTargets(const GfxRenderTargetSetup& rt);
	GFX_API void ResolveColorSurface (RenderSurfaceHandle srcHandle, RenderSurfaceHandle dstHandle);
	GFX_API void ResolveDepthIntoTexture (RenderSurfaceHandle colorHandle, RenderSurfaceHandle depthHandle);

	GFX_API RenderSurfaceBase*	AllocRenderSurface(bool colorSurface);
	GFX_API	void				CopyRenderSurfaceDesc(RenderSurfaceBase* dst, const RenderSurfaceBase* src);
	GFX_API void				DeallocRenderSurface(RenderSurfaceBase* rs);

	GFX_API RenderSurfaceHandle GetActiveRenderColorSurface (int index) const;
	GFX_API RenderSurfaceHandle GetActiveRenderDepthSurface () const;
	GFX_API int GetActiveRenderTargetCount () const;

	GFX_API int GetActiveRenderSurfaceWidth() const;
	GFX_API int GetActiveRenderSurfaceHeight() const;

	GFX_API bool IsRenderTargetConfigValid(UInt32 width, UInt32 height, RenderTextureFormat colorFormat, DepthBufferFormat depthFormat);

	GFX_API void SetSurfaceFlags(RenderSurfaceHandle surf, UInt32 flags, UInt32 keepFlags);

	GFX_API void RegisterNativeTexture( TextureID texture, intptr_t nativeTex, TextureDimension dim );
	GFX_API void UnregisterNativeTexture( TextureID texture );

	GFX_API void UploadTexture2D( TextureID texture, TextureDimension dimension, const UInt8* srcData, int srcSize, int width, int height, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureUsageMode usageMode, TextureColorSpace colorSpace );
	GFX_API void UploadTextureSubData2D( TextureID texture, const UInt8* srcData, int srcSize, int mipLevel, int x, int y, int width, int height, TextureFormat format, TextureColorSpace colorSpace );
	GFX_API void UploadTextureCube( TextureID texture, const UInt8* srcData, int srcSize, int faceDataSize, int size, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureColorSpace colorSpace );
	GFX_API void UploadTexture3D( TextureID texture, const UInt8* srcData, int srcSize, int width, int height, int depth, TextureFormat format, int mipCount, UInt32 uploadFlags );
	GFX_API void DeleteTexture( TextureID texture );

	GFX_API void UploadTexture2DArray(TextureID texture, const UInt8* srcData, size_t elementSize, int width, int height, int depth, TextureFormat format, int mipCount, UInt32 uploadFlags, TextureColorSpace colorSpace);

	GFX_API SparseTextureInfo CreateSparseTexture (TextureID texture, int width, int height, TextureFormat format, int mipCount, TextureColorSpace colorSpace);
	GFX_API void UploadSparseTextureTile (TextureID texture, int tileX, int tileY, int mip, const UInt8* srcData, int srcSize, int srcPitch);

	GFX_API void CopyTexture(TextureID src, TextureID dst);
	GFX_API void CopyTexture(TextureID src, int srcElement, int srcMip, int srcMipCount, TextureID dst, int dstElement, int dstMip, int dstMipCount);
	GFX_API void CopyTexture(TextureID src, int srcElement, int srcMip, int srcMipCount, int srcX, int srcY, int srcWidth, int srcHeight, TextureID dst, int dstElement, int dstMip, int dstMipCount, int dstX, int dstY);

	GFX_API PresentMode	GetPresentMode();

	GFX_API void	BeginFrame();
	GFX_API void	EndFrame();
	GFX_API void	PresentFrame();
	GFX_API void	PresentFrame(const ChannelAssigns* blitChannels);
	GFX_API void	SwapDynamicVBOBuffers(UInt32 frameIndex);
	GFX_API bool	IsValidState();
	GFX_API bool	HandleInvalidState();
	GFX_API void	ResetDynamicResources();
	GFX_API void	Flush();
	GFX_API void	FinishRendering();
	GFX_API UInt32	InsertCPUFence();
	GFX_API UInt32	GetNextCPUFence();
	GFX_API void	WaitOnCPUFence(UInt32 fence);

	GFX_API void	AcquireThreadOwnership();
	GFX_API void	ReleaseThreadOwnership();

	GFX_API void	ImmediateVertex( float x, float y, float z );
	GFX_API void	ImmediateNormal( float x, float y, float z );
	GFX_API void	ImmediateColor( float r, float g, float b, float a );
	GFX_API void	ImmediateTexCoordAll( float x, float y, float z );
	GFX_API void	ImmediateTexCoord( int unit, float x, float y, float z );
	GFX_API void	ImmediateBegin( GfxPrimitiveType type, const ChannelAssigns* channelAssigns = NULL );
	GFX_API	void	ImmediateEnd();

	// Recording display lists
	GFX_API bool	BeginRecording();
	GFX_API bool	EndRecording( GfxDisplayList** outDisplayList, const ShaderPropertySheet& globalProperties );

	// Capturing screen shots / blits
	GFX_API bool	CaptureScreenshot( int left, int bottom, int width, int height, UInt8* rgba32 );
	GFX_API bool	ReadbackImage( ImageReference& image, int left, int bottom, int width, int height, int destX, int destY );
	GFX_API void	GrabIntoRenderTexture (RenderSurfaceHandle rs, RenderSurfaceHandle rd, int x, int y, int width, int height);

	// Any housekeeping around draw calls
	GFX_API void	BeforeDrawCall();

	GFX_API void	SetActiveContext (void* ctx);

	GFX_API void	ResetFrameStats();
	GFX_API void	BeginFrameStats();
	GFX_API void	EndFrameStats();
	GFX_API void	SaveDrawStats();
	GFX_API void	RestoreDrawStats();
	GFX_API void	SynchronizeStats();

	GFX_API void	SetGlobalDepthBias (float bias, float slopeBias);

	GFX_API void* GetNativeGfxDevice();
	GFX_API void* GetNativeTexturePointer(TextureID id);
	GFX_API RenderSurfaceBase* GetNativeRenderSurfacePointer(RenderSurfaceBase* rs);

	GFX_API void InsertCustomMarker (int eventId);
	GFX_API void InsertCustomMarkerCallback (UnityRenderingEvent callback, int eventId);

	GFX_API void SetComputeBufferData (ComputeBufferID bufferHandle, const void* data, size_t size);
	GFX_API void GetComputeBufferData (ComputeBufferID bufferHandle, void* dest, size_t destSize);
	GFX_API void SetComputeBufferCounterValue (ComputeBufferID uav, UInt32 counterValue);
	GFX_API void CopyComputeBufferCount (ComputeBufferID srcBuffer, ComputeBufferID dstBuffer, UInt32 dstOffset);

	GFX_API void SetRandomWriteTargetTexture (int index, TextureID tid);
	GFX_API void SetRandomWriteTargetBuffer (int index, ComputeBufferID bufferHandle);
	GFX_API void ClearRandomWriteTargets ();

	GFX_API bool HasActiveRandomWriteTarget () const;

	GFX_API ComputeProgramHandle CreateComputeProgram (const UInt8* code, size_t codeSize);
	GFX_API void DestroyComputeProgram (ComputeProgramHandle& cpHandle);
	GFX_API void ResolveComputeProgramResources(ComputeProgramHandle cpHandle, ComputeShaderKernel& kernel, std::vector<ComputeShaderCB>& constantBuffers, std::vector<ComputeShaderParam>& uniforms, bool preResolved);
	GFX_API void CreateComputeConstantBuffers (unsigned count, const UInt32* sizes, ConstantBufferHandle* outCBs);
	GFX_API void DestroyComputeConstantBuffers (unsigned count, ConstantBufferHandle* cbs);
	GFX_API void CreateComputeBuffer (ComputeBufferID id, size_t count, size_t stride, UInt32 flags);
	GFX_API void DestroyComputeBuffer (ComputeBufferID handle);
	GFX_API void SetComputeProgram(ComputeProgramHandle /*cpHandle*/);
	GFX_API void UpdateComputeConstantBuffers (unsigned count, ConstantBufferHandle* cbs, UInt32 cbDirty, size_t dataSize, const UInt8* data, const UInt32* cbSizes, const UInt32* cbOffsets, const int* bindPoints);
	GFX_API void UpdateComputeResources (
		unsigned texCount, const TextureID* textures, const TextureDimension* texDims, const int* texBindPoints,
		unsigned samplerCount, const unsigned* samplers,
		unsigned inBufferCount, const ComputeBufferID* inBuffers, const int* inBufferBindPoints, const ComputeBufferCounter* inBufferCounters,
		unsigned outBufferCount, const ComputeBufferID* outBuffers, const TextureID* outTextures, const TextureDimension* outTexDims, const UInt32* outBufferBindPoints, const ComputeBufferCounter* outBufferCounters);

	GFX_API void DispatchComputeProgram (ComputeProgramHandle cpHandle, unsigned threadGroupsX, unsigned threadGroupsY, unsigned threadGroupsZ);
	GFX_API void DispatchComputeProgram (ComputeProgramHandle cpHandle, ComputeBufferID indirectBuffer, UInt32 argsOffset);

	GFX_API void DrawNullGeometry (GfxPrimitiveType topology, int vertexCount, int instanceCount);
	GFX_API void DrawNullGeometryIndirect (GfxPrimitiveType topology, ComputeBufferID bufferHandle, UInt32 bufferOffset);

	GFX_API void SetStereoTarget (StereoscopicEye eye);
	GFX_API void SetSinglePassStereo(SinglePassStereo mode);
	GFX_API void SendVRDeviceEvent (UInt32 eventID, UInt32 data = 0);

	GFX_API size_t GetMultiGPUCount ();
	GFX_API size_t GetCurrentGPU ();

#if ENABLE_PROFILER
	GFX_API void	BeginProfileEvent (const char* name);
	GFX_API void	EndProfileEvent ();
	GFX_API void	ProfileControl (GfxProfileControl ctrl, unsigned param);
	GFX_API GfxTimerQuery* CreateTimerQuery();
	GFX_API void	DeleteTimerQuery(GfxTimerQuery* query);
	GFX_API void	BeginTimerQueries();
	GFX_API void	EndTimerQueries();
#endif // #if ENABLE_PROFILER

#if GFX_SUPPORTS_THREADED_CLIENT_PROCESS
	GFX_API void	ExecuteAsync(int count, GfxDeviceAsyncCommand::Func* func, GfxDeviceAsyncCommand::ArgScratch** scratches, const GfxDeviceAsyncCommand::Arg* arg, JobFence& depends);
#endif

	// Editor-only stuff
#if UNITY_EDITOR
	GFX_API void	SetAntiAliasFlag( bool aa );
	GFX_API int		GetCurrentTargetAA() const;
#endif

#if FRAME_DEBUGGER_AVAILABLE
	GFX_API void SetNextShaderInfo (int shaderInstanceID, int passIndex);
	GFX_API void SetNextDrawCallInfo (int rendererInstanceID, int meshInstanceID, int meshSubset);

	GFX_API void BeginGameViewRenderingOffscreen();
	GFX_API void EndGameViewRenderingOffscreen();

	GFX_API void BeginGameViewRendering();
	GFX_API void EndGameViewRendering();
#endif // #if FRAME_DEBUGGER_AVAILABLE

#if UNITY_EDITOR && UNITY_WIN
	//ToDo: This is windows specific code, we should replace HWND window with something more abstract
	GFX_API GfxDeviceWindow* CreateGfxWindow( HWND window, int width, int height, DepthBufferFormat depthFormat, int antiAlias );

	void SetActiveWindow(ClientDeviceWindow* handle);
	void WindowReshape(ClientDeviceWindow* handle, int width, int height, DepthBufferFormat depthFormat, int antiAlias);
	void WindowDestroy(ClientDeviceWindow* handle);
	void BeginRendering(ClientDeviceWindow* handle);
	void EndRendering(ClientDeviceWindow* handle, bool presentContent);
#endif

#if SUPPORT_MULTIPLE_DISPLAYS
	GFX_API bool	SetDisplayTarget(const UInt32 displayId);
#endif

#if GFX_OPENGLESxx_ONLY
	GFX_API void ReloadResources() {};
#endif

#if GFX_HAS_OBJECT_LABEL
	GFX_API void	SetTextureName(TextureID tex, const char* name);
	GFX_API void	SetRenderSurfaceName(RenderSurfaceBase* rs, const char* name);
	GFX_API void	SetBufferName(GfxBuffer* buffer, const char* name);
	GFX_API void	SetGpuProgramName(GpuProgram* prog, const char* name);
#endif

	void SendInvertProjectionMatrixCommand( bool enable );
	void RestoreCommandQueueValues();

	
	bool IsThreaded() const { return m_Threaded; }
	bool IsSerializing() const { return m_Serialize; }

	void SubmitCleanupToQueue(ThreadedStreamBuffer* tsb);
	ThreadedStreamBuffer* GetCommandQueue() const { return m_CommandQueue; }
	GfxDeviceWorker* GetGfxDeviceWorker() const { return m_DeviceWorker; }
	void OverrideGfxDeviceWorker(GfxDeviceWorker* worker) { m_DeviceWorker = worker; m_DeviceWorkerOverride = true; }

	void SetRealGfxDevice(GfxThreadableDevice* realDevice);
	void WriteBufferData(const void* data, int size, bool asPointer = false);
	void SubmitCommands();
	GFX_API void WakeRenderThread();
	void DoLockstep();

	GFX_API RenderTextureFormat	GetDefaultRTFormat() const;
	GFX_API RenderTextureFormat	GetDefaultHDRRTFormat() const;

	ThreadedStreamBuffer* GetMainCommandQueue() const { return m_MainThreadCommandQueue; }
	void AllocCommandQueue();
	void ReleaseCommandQueue();
	GFX_API void AsyncResourceUpload(const int timeSliceMS, const int bufferSizeSetting);
	GFX_API void SyncAsyncResourceUpload(const TextureID texId, AsyncFence fence, const int bufferSizeSetting);

	// This will copy whole platform render surface descriptor (and not just store argument pointer)
	GFX_API void SetBackBufferColorDepthSurface(RenderSurfaceBase* color, RenderSurfaceBase* depth);

#if GFX_SUPPORTS_OPENGL_UNIFIED
	GFX_API GfxDeviceLevelGL GetDeviceLevel() const { return m_RealDevice ? m_RealDevice->GetDeviceLevel(): kGfxLevelUninitialized; }
#endif//GFX_SUPPORTS_OPENGL_UNIFIED


protected:
	GFX_API DynamicVBO*	CreateDynamicVBO();

	GFX_API size_t	RenderSurfaceStructMemorySize(bool colorSurface);
	GFX_API void	AliasRenderSurfacePlatform(RenderSurfaceBase* rs, TextureID origTexID);
	GFX_API bool	CreateColorRenderSurfacePlatform(RenderSurfaceBase* rs, RenderTextureFormat format);
	GFX_API bool	CreateDepthRenderSurfacePlatform(RenderSurfaceBase* rs, DepthBufferFormat format);
	GFX_API void	DestroyRenderSurfacePlatform(RenderSurfaceBase* rs);

private:
	friend class ThreadedDisplayList;
	void UpdateShadersActive(bool shadersActive[kShaderTypeCount]);

private:
	void BeforeRenderTargetChange(int count, RenderSurfaceHandle* colorHandles, RenderSurfaceHandle depthHandle);
	void AfterRenderTargetChange();
	void WaitForPendingPresent();
	void WaitForSignal();

	typedef std::map< GfxBlendState, ClientDeviceBlendState, memcmp_less<GfxBlendState> > CachedBlendStates;
	typedef std::map< GfxDepthState, ClientDeviceDepthState, memcmp_less<GfxDepthState> > CachedDepthStates;
	typedef std::map< GfxStencilState, ClientDeviceStencilState, memcmp_less<GfxStencilState> > CachedStencilStates;
	typedef std::map< GfxRasterState, ClientDeviceRasterState, memcmp_less<GfxRasterState> > CachedRasterStates;

	enum
	{
		kMaxCallDepth = 1
	};

	UInt32				m_CreateFlags;
	GfxDeviceWorker*    m_DeviceWorker;
	GfxThreadableDevice* m_RealDevice;
	bool				m_Threaded;
	bool				m_Serialize;
	int					m_RecordDepth;
	
	ThreadedStreamBuffer* m_CommandQueue;
	static ThreadedStreamBuffer* m_MainThreadCommandQueue;

	DisplayListContext	m_DisplayListStack[kMaxCallDepth + 1];
	DisplayListContext*	m_CurrentContext;
	CachedBlendStates	m_CachedBlendStates;
	CachedDepthStates	m_CachedDepthStates;
	CachedStencilStates	m_CachedStencilStates;
	CachedRasterStates	m_CachedRasterStates;
	RectInt				m_Viewport;
#if GFX_SUPPORTS_SINGLE_PASS_STEREO
	enum { kStereoDisabled, kStereoEnabled, kStereoEnabledCount };
	RectInt				m_StereoViewports[kStereoscopicEyeCount];
#endif
	RectInt				m_ScissorRect;
	int					m_ScissorEnabled;
	RenderSurfaceHandle m_ActiveRenderColorSurfaces[kMaxSupportedRenderTargets];
	RenderSurfaceHandle m_ActiveRenderDepthSurface;
	int					m_ActiveRenderTargetCount;
	bool				m_ActiveRandomWriteTargets;
	int					m_ThreadOwnershipCount;
	UInt32				m_CurrentCPUFence;
	UInt32				m_CurrentGPU;
	UInt32				m_PresentFrameID;
	//UInt32				m_SkinMeshIDAllocator;
	bool				m_PresentPending;
	bool				m_Wireframe;
	bool				m_sRGBWrite;
	bool				m_DeviceWorkerOverride;
	dynamic_array<UInt8> m_ReadbackData;
};

#endif
