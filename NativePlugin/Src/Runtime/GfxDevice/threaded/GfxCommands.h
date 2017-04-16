#pragma once

#include "Runtime/GfxDevice/GfxDevice.h"
#include "Runtime/GfxDevice/ChannelAssigns.h"
#include "Runtime/GfxDevice/threaded/ThreadedDeviceStates.h"
#include "Runtime/Graphics/Mesh/VBO.h" // for VertexBufferData
#include "Runtime/Graphics/Mesh/DynamicVBO.h"
#include "Runtime/Video/BaseVideoTexture.h"

class ThreadedBuffer;
class ThreadedVertexDeclaration;

enum GfxCommand
{
	kGfxCmd_Unused = 10000,
	kGfxCmd_InvalidateState,
	kGfxCmd_VerifyState,
	kGfxCmd_SetMaxBufferedFrames,
	kGfxCmd_Clear,
	kGfxCmd_SetUserBackfaceMode,
	kGfxCmd_SetForceCullMode,
	kGfxCmd_SetInvertProjectionMatrix,
	kGfxCmd_RenderTargetBarrier,
	kGfxCmd_CreateBlendState,
	kGfxCmd_CreateDepthState,
	kGfxCmd_CreateStencilState,
	kGfxCmd_CreateRasterState,
	kGfxCmd_SetBlendState,
	kGfxCmd_SetDepthState,
	kGfxCmd_SetStencilState,
	kGfxCmd_SetRasterState,
	kGfxCmd_SetSRGBWrite,
	kGfxCmd_SetWorldMatrix,
	kGfxCmd_SetViewMatrix,
	kGfxCmd_SetProjectionMatrix,
	kGfxCmd_UpdateViewProjectionMatrix,
	kGfxCmd_SetStereoMatrix,
	kGfxCmd_SetSinglePassStereoEyeMask,
	kGfxCmd_SaveStereoConstants,
	kGfxCmd_RestoreStereoConstants,
	kGfxCmd_SetBackfaceMode,
	kGfxCmd_SetViewport,
	kGfxCmd_SetStereoViewport,
	kGfxCmd_SetStereoScissorRects,
	kGfxCmd_SetScissorRect,
	kGfxCmd_DisableScissor,
	kGfxCmd_SetTextures,
	kGfxCmd_SetTextureParams,
	kGfxCmd_SetShaderPropertiesCopied,
	kGfxCmd_SetShaderPropertiesShared,
	kGfxCmd_SetShaders,
	kGfxCmd_DestroySubProgram,
	kGfxCmd_UpdateConstantBuffer,
	kGfxCmd_SetStereoConstantBuffers,
	kGfxCmd_EndGeometryJobFrame,
	kGfxCmd_PutGeometryJobFence,
	kGfxCmd_ScheduleGeometryJobs,
	kGfxCmd_ScheduleDynamicVBOGeometryJobs,
	kGfxCmd_AddBatchStats,
	kGfxCmd_BeginDynamicBatching,
	kGfxCmd_DynamicBatchMesh,
	kGfxCmd_EndDynamicBatching,
	kGfxCmd_AddSetPassStats,
	kGfxCmd_ReleaseSharedMeshData,
	kGfxCmd_ReleaseSharedTextureData,
	kGfxCmd_ReleaseAsyncCommandHeader,
	kGfxCmd_CreateGPUSkinPoseBuffer,
	kGfxCmd_DeleteGPUSkinPoseBuffer,
	kGfxCmd_UpdateSkinPoseBuffer,
	kGfxCmd_SkinOnGPU,
	kGfxCmd_CreateRenderColorSurface,
	kGfxCmd_CreateRenderDepthSurface,
	kGfxCmd_AliasRenderSurface,
	kGfxCmd_DestroyRenderSurface,
	kGfxCmd_DiscardContents,
	kGfxCmd_SetRenderTarget,
	kGfxCmd_ResolveColorSurface,
	kGfxCmd_ResolveDepthIntoTexture,
	kGfxCmd_SetSurfaceFlags,
	kGfxCmd_RegisterNativeTexture,
	kGfxCmd_UnregisterNativeTexture,
	kGfxCmd_UploadTexture2D,
	kGfxCmd_UploadTextureSubData2D,
	kGfxCmd_UploadTextureCube,
	kGfxCmd_UploadTexture3D,
	kGfxCmd_UploadTexture2DArray,
	kGfxCmd_DeleteTexture,
	kGfxCmd_CreateSparseTexture,
	kGfxCmd_UploadSparseTextureTile,
	kGfxCmd_CopyTextureWhole,
	kGfxCmd_CopyTextureSlice,
	kGfxCmd_CopyTextureRegion,
	kGfxCmd_BeginFrame,
	kGfxCmd_EndFrame,
	kGfxCmd_PresentFrame,
	kGfxCmd_HandleInvalidState,
	kGfxCmd_Flush,
	kGfxCmd_FinishRendering,
	kGfxCmd_InsertCPUFence,
	kGfxCmd_ImmediateVertex,
	kGfxCmd_ImmediateNormal,
	kGfxCmd_ImmediateColor,
	kGfxCmd_ImmediateTexCoordAll,
	kGfxCmd_ImmediateTexCoord,
	kGfxCmd_ImmediateBegin,
	kGfxCmd_ImmediateEnd,
	kGfxCmd_CaptureScreenshot,
	kGfxCmd_ReadbackImage,
	kGfxCmd_GrabIntoRenderTexture,
	kGfxCmd_SetActiveContext,
	kGfxCmd_ResetFrameStats,
	kGfxCmd_BeginFrameStats,
	kGfxCmd_EndFrameStats,
	kGfxCmd_SaveDrawStats,
	kGfxCmd_RestoreDrawStats,
	kGfxCmd_SynchronizeStats,
	kGfxCmd_SetGlobalBias,
	kGfxCmd_SetAntiAliasFlag,
	kGfxCmd_SetWireframe,
	kGfxCmd_Quit,

	kGfxCmd_CreateIndexBuffer,
	kGfxCmd_CreateVertexBuffer,
	kGfxCmd_DeleteBuffer,
	kGfxCmd_UpdateBuffer,
	kGfxCmd_EndBufferWrite,
	kGfxCmd_GetVertexDeclaration,
	kGfxCmd_DrawBuffers,

	kGfxCmd_DynVBO_Chunk,
	kGfxCmd_DynVBO_DrawChunk,
	kGfxCmd_DynVBO_SwapBuffers,

	kGfxCmd_DisplayList_Call,
	kGfxCmd_DisplayList_End,

	kGfxCmd_CreateWindow,
	kGfxCmd_SetActiveWindow,
	kGfxCmd_WindowReshape,
	kGfxCmd_WindowDestroy,
	kGfxCmd_BeginRendering,
	kGfxCmd_EndRendering,

	kGfxCmd_AllocRenderSurface,
	kGfxCmd_CopyRenderSurfaceDesc,
	kGfxCmd_DeallocRenderSurface,

	kGfxCmd_AcquireThreadOwnership,
	kGfxCmd_ReleaseThreadOwnership,

	kGfxCmd_BeginProfileEvent,
	kGfxCmd_EndProfileEvent,
	kGfxCmd_ProfileControl,
	kGfxCmd_BeginTimerQueries,
	kGfxCmd_EndTimerQueries,

	kGfxCmd_TimerQuery_Constructor,
	kGfxCmd_TimerQuery_Destructor,
	kGfxCmd_TimerQuery_Measure,
	kGfxCmd_TimerQuery_GetElapsed,

	kGfxCmd_InsertCustomMarker,
	kGfxCmd_InsertCustomMarkerCallback,

	kGfxCmd_SetComputeBufferData,
	kGfxCmd_GetComputeBufferData,
	kGfxCmd_SetComputeBufferCounterValue,
	kGfxCmd_CopyComputeBufferCount,
	kGfxCmd_SetRandomWriteTargetTexture,
	kGfxCmd_SetRandomWriteTargetBuffer,
	kGfxCmd_ClearRandomWriteTargets,
	kGfxCmd_CreateComputeProgram,
	kGfxCmd_DestroyComputeProgram,
	kGfxCmd_ResolveComputeProgramResources,
	kGfxCmd_CreateComputeConstantBuffers,
	kGfxCmd_DestroyComputeConstantBuffers,
	kGfxCmd_CreateComputeBuffer,
	kGfxCmd_DestroyComputeBuffer,
	kGfxCmd_UpdateComputeConstantBuffers,
	kGfxCmd_SetComputeProgram,
	kGfxCmd_UpdateComputeResources,
	kGfxCmd_DispatchComputeProgram,
	kGfxCmd_DispatchComputeProgramIndirect,
	kGfxCmd_DrawNullGeometry,
	kGfxCmd_DrawNullGeometryIndirect,
	kGfxCmd_SetGpuProgramParameters,
	kGfxCmd_DestroyGpuProgram,
	kGfxCmd_SetStereoTarget,
	kGfxCmd_SetSinglePassStereo,
	kGfxCmd_SendVRDeviceEvent,

	kGfxCmd_CommandQueue_Call,
	kGfxCmd_CommandQueue_ReturnAndFree,

	kGfxCmd_DequeueCreateGpuProgramQueue,

#if GFX_HAS_OBJECT_LABEL
	kGfxCmd_SetTextureName,
	kGfxCmd_SetRenderSurfaceName,
	kGfxCmd_SetBufferName,
	kGfxCmd_SetGpuProgramName,
#endif

#if SUPPORT_MULTIPLE_DISPLAYS
	kGfxCmd_SetDisplayTarget,
#endif

	kGfxCmd_AsyncResourceUpload,
	kGfxCmd_SyncAsyncResourceUpload,

	kGfxCmd_SetBackBufferColorDepthSurface,
	// Platform specific flags should live in GfxCommandsExt.h
	// in the PlatformSpecific folders and not pollute the core
	// system.

	kGfxCmd_Count
};

typedef int GfxCmdBool;


struct GfxCmdClear
{
	UInt32 clearFlags;
	ColorRGBAf color;
	float depth;
	UInt32 stencil;
};


struct GfxCmdSetTextures
{
	ShaderType shaderType;
	int count;
};

struct GfxCmdSetTextureParams
{
	TextureID texture;
	TextureDimension texDim;
	TextureFilterMode filter;
	TextureWrapMode wrap;
	int anisoLevel;
	float mipBias;
	bool hasMipMap;
	TextureColorSpace colorSpace;
	ShadowSamplingMode shadowSamplingMode;
};

template <typename ObjectT>
struct GfxCmdSetObjectName
{
	ObjectT	object;
	int nameLength;
};

typedef GfxCmdSetObjectName<TextureID>					GfxCmdSetTextureName;
typedef GfxCmdSetObjectName<ClientDeviceRenderSurface*>	GfxCmdSetRenderSurfaceName;
typedef GfxCmdSetObjectName<ThreadedBuffer*>			GfxCmdSetBufferName;
typedef GfxCmdSetObjectName<GpuProgram*>				GfxCmdSetGpuProgramName;

struct GfxCmdSetShaders
{
	GpuProgram* programs[kShaderTypeCount];
	const GpuProgramParameters* params[kShaderTypeCount];
};

struct GfxCmdUpdateConstantBuffer
{
	int id;
	size_t size;
	bool hasData;
};

struct GfxCmdSetStereoConstantBuffers
{
	int leftID;
	int rightID;
	int monoscopicID;
	size_t size;
};

struct GfxCmdAddBatchStats
{
	GfxBatchStatsType type;
	int batchedTris;
	int batchedVerts;
	int batchedCalls;
	TimeFormat batchedDt;
};

struct GfxCmdBeginDynamicBatching
{
	ChannelAssigns shaderChannels;
	UInt32 channelsMask;
	UInt32 stride;
	ThreadedVertexDeclaration* vertexDeclaration;
	size_t maxVertices;
	size_t maxIndices;
	GfxPrimitiveType topology;
};

struct GfxCmdDynamicBatchMesh
{
	Matrix4x4f matrix;
	VertexBufferData vertices;
	UInt32 firstVertex;
	UInt32 vertexCount;
	const UInt16* indices;
	UInt32 indexCount;
	GfxTransformVerticesFlags transformFlags;
};

struct GfxCmdSkinOnGPU
{
	VertexStreamSource sourceMesh;
	GfxBuffer* skinBuffer;
	GPUSkinPoseBuffer* poseBuffer;
	GfxBuffer* destBuffer;
	int vertexCount;
	int bonesPerVertex;
	UInt32 channelMask;
	bool lastThisFrame;
};

struct GfxCmdCreateRenderColorSurface
{
	RenderTextureFormat format;
};

struct GfxCmdCreateRenderDepthSurface
{
	DepthBufferFormat format;
};

struct GfxCmdSetRenderTarget
{
	RenderSurfaceHandle colorHandles[kMaxSupportedRenderTargets];
	RenderSurfaceHandle depthHandle;
	int			colorCount;
	int			mipLevel;
	CubemapFace face;
};

struct GfxCmdResolveDepthIntoTexture
{
	RenderSurfaceHandle colorHandle;
	RenderSurfaceHandle depthHandle;
};

struct GfxCmdSetSurfaceFlags
{
	RenderSurfaceHandle surf;
	UInt32 flags;
	UInt32 keepFlags;
};

struct GfxCmdRegisterNativeTexture
{
	TextureID texture;
	intptr_t nativeTex;
	TextureDimension dimension;
};

struct GfxCmdUploadTexture2D
{
	TextureID texture;
	TextureDimension dimension;
	int srcSize;
	int width;
	int height;
	TextureFormat format;
	int mipCount;
	UInt32 uploadFlags;
	TextureUsageMode usageMode;
	TextureColorSpace colorSpace;
};

struct GfxCmdUploadTextureSubData2D
{
	TextureID texture;
	int srcSize;
	int mipLevel;
	int x;
	int y;
	int width;
	int height;
	TextureFormat format;
	TextureColorSpace colorSpace;
};

struct GfxCmdUploadTextureCube
{
	TextureID texture;
	int srcSize;
	int faceDataSize;
	int size;
	TextureFormat format;
	int mipCount;
	UInt32 uploadFlags;
	TextureColorSpace colorSpace;
};

struct GfxCmdUploadTexture3D
{
	TextureID texture;
	int srcSize;
	int width;
	int height;
	int depth;
	TextureFormat format;
	int mipCount;
	UInt32 uploadFlags;
};

struct GfxCmdUploadTexture2DArray
{
	TextureID texture;
	size_t elementSize;
	int width;
	int height;
	int depth;
	TextureFormat format;
	int mipCount;
	UInt32 uploadFlags;
	TextureColorSpace colorSpace;
};

struct GfxCmdCopyTextureWhole
{
	TextureID src;
	TextureID dst;
};

struct GfxCmdCopyTextureSlice
{
	TextureID src;
	int srcElement;
	int srcMip;
	int srcMipCount;
	TextureID dst;
	int dstElement;
	int dstMip;
	int dstMipCount;
};

struct GfxCmdCopyTextureRegion
{
	TextureID src;
	int srcElement;
	int srcMip;
	int srcMipCount;
	int srcX;
	int srcY;
	int srcWidth;
	int srcHeight;
	TextureID dst;
	int dstElement;
	int dstMip;
	int dstMipCount;
	int dstX;
	int dstY;
};


struct GfxCmdUpdateBuffer
{
	ThreadedBuffer* threadedBuffer;
	GfxBufferMode mode;
	GfxBufferLabel label;
	size_t size;
	UInt32 flags;
	GfxCmdBool hasData;
};

struct GfxCmdEndBufferWrite
{
	ThreadedBuffer* threadedBuffer;
	size_t offset;
	size_t size;
};

struct GfxCmdDrawBuffers
{
	ThreadedBuffer* indexBuf;
	int vertexStreamCount;
	int drawRangeCount;
	ThreadedVertexDeclaration* vertexDecl;
	ChannelAssigns channels;
};

struct GfxCmdCreateSparseTexture
{
	TextureID texture;
	int width;
	int height;
	TextureFormat format;
	int mipCount;
	TextureColorSpace colorSpace;
	SparseTextureInfo* returnInfo;
};

struct GfxCmdUploadSparseTextureTile
{
	TextureID texture;
	int tileX;
	int tileY;
	int mip;
	int srcSize;
	int srcPitch;
};

struct GfxCmdDrawUserPrimitives
{
	GfxPrimitiveType type;
	int vertexCount;
	UInt32 vertexChannels;
	int stride;
};

struct GfxCmdVector3
{
	float x;
	float y;
	float z;
};

struct GfxCmdVector4
{
	float x;
	float y;
	float z;
	float w;
};

struct GfxCmdImmediateTexCoord
{
	int unit;
	float x;
	float y;
	float z;
};

struct GfxCmdImmediateBegin
{
	GfxPrimitiveType primType;
	bool hasChannelAssigns;
};

struct GfxCmdCaptureScreenshot
{
	int left;
	int bottom;
	int width;
	int height;
	UInt8* rgba32;
	bool* success;
};

struct GfxCmdReadbackImage
{
	ImageReference& image;
	int left;
	int bottom;
	int width;
	int height;
	int destX;
	int destY;
	bool* success;
};

struct GfxCmdGrabIntoRenderTexture
{
	RenderSurfaceHandle rs;
	RenderSurfaceHandle rd;
	int x;
	int y;
	int width;
	int height;
};

struct GfxCmdDynVboChunk
{
	UInt32 vertexStride;
	UInt32 actualVertices;
	UInt32 actualIndices;
	GfxPrimitiveType primType;
	DynamicVBOChunkHandle chunkHandle;
};

struct GfxCmdUploadYuvFrame
{
	YuvFrame	yuv;
	TextureID	id;
	void*		userData;
};

struct GfxCmdDynVboDrawChunk
{
	ChannelAssigns channels;
	UInt32 channelsMask;
	VertexDeclaration* vertexDecl;
	DynamicVBOChunkHandle vboChunk;
	int numDrawParams;
};

struct GfxCmdPresentFrame
{
	bool signalEvent;
	bool hasBlitChannels;
	UInt32 presentFrameID;
};

struct GfxCmdResolveComputeProgramResources
{
	ClientDeviceComputeProgram* handle;
	ComputeShaderKernel& kernel;
	std::vector<ComputeShaderCB>& constantBuffers;
	std::vector<ComputeShaderParam>& uniforms;
	bool preResolved;
};

struct GfxCmdSendVRDeviceEvent
{
	UInt32 eventID;
	UInt32 data;
};

#if UNITY_WIN && UNITY_EDITOR

struct GfxCmdWindowReshape
{
	int width;
	int height;
	DepthBufferFormat depthFormat;
	int antiAlias;
};

struct GfxCmdCreateWindow
{
	HWND window;
	int width;
	int height;
	DepthBufferFormat depthFormat;
	int antiAlias;
};

#endif
