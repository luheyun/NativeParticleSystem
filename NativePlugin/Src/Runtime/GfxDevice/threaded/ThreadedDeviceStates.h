#pragma once
#if ENABLE_MULTITHREADED_CODE

#include "Runtime/GfxDevice/GfxDeviceObjects.h"
#include "Runtime/GfxDevice/GfxDeviceResources.h"
#include "Runtime/GfxDevice/GPUSkinPoseBuffer.h"
#include "Runtime/Graphics/Mesh/VertexData.h"
#include "Runtime/Graphics/RenderSurface.h"

class GfxTimerQuery;
class GfxDeviceWindow;

struct ClientDeviceBlendState : public DeviceBlendState
{
	ClientDeviceBlendState(const GfxBlendState& src) : DeviceBlendState(src), internalState(NULL) {}
	const DeviceBlendState* internalState;
};

struct ClientDeviceDepthState : public DeviceDepthState
{
	ClientDeviceDepthState(const GfxDepthState& src) : DeviceDepthState(src), internalState(NULL) {}
	const DeviceDepthState* internalState;
};

struct ClientDeviceStencilState : public DeviceStencilState
{
	ClientDeviceStencilState(const GfxStencilState& src) : DeviceStencilState(src), internalState(NULL) {}
	const DeviceStencilState* internalState;
};

struct ClientDeviceRasterState : public DeviceRasterState
{
	ClientDeviceRasterState(const GfxRasterState& src) : DeviceRasterState(src), internalState(NULL) {}
	const DeviceRasterState* internalState;
};

struct ClientDeviceRenderSurface : RenderSurfaceBase
{
	enum SurfaceState { kInitial, kCleared, kRendered, kResolved };

	ClientDeviceRenderSurface(int w, int h, TextureDimension texDim)
	{
		RenderSurfaceBase_Init(*this);
		width = w;
		height = h;
		dim = texDim;
		zformat = kDepthFormatNone;
		state = kInitial;
	}

	RenderSurfaceHandle internalHandle;
	DepthBufferFormat zformat;
	SurfaceState state;
};

struct ClientDeviceTimerQuery
{
	ClientDeviceTimerQuery() : internalQuery(NULL), elapsed(0), pending(false) {}
	GfxTimerQuery* GetInternal() { return const_cast<GfxTimerQuery*>(internalQuery); }
	volatile GfxTimerQuery* internalQuery;
	volatile UInt64 elapsed;
	volatile bool pending;
};

struct ClientDeviceWindow
{
	ClientDeviceWindow() : internalWindow(NULL) {}
	GfxDeviceWindow* GetInternal() { return const_cast<GfxDeviceWindow*>(internalWindow); }
	volatile GfxDeviceWindow* internalWindow;
};

struct ClientDeviceConstantBuffer
{
	ClientDeviceConstantBuffer(UInt32 sz) : size(sz) {}
	ConstantBufferHandle internalHandle;
	UInt32 size;
};

struct ClientDeviceComputeProgram
{
	ClientDeviceComputeProgram() {}
	ComputeProgramHandle internalHandle;
};

class ThreadedGPUSkinPoseBuffer : public GPUSkinPoseBuffer
{
public:
	ThreadedGPUSkinPoseBuffer() : realBuf(NULL) {}

	void SetBoneCount(int count) { m_BoneCount = count; }

	GPUSkinPoseBuffer* volatile realBuf;
};

inline GPUSkinPoseBuffer* GetRealBuffer(GPUSkinPoseBuffer* threadedBuffer)
{
	return static_cast<ThreadedGPUSkinPoseBuffer*>(threadedBuffer)->realBuf;
}

#endif
