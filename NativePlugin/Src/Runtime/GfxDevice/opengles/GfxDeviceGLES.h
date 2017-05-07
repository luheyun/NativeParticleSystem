#pragma once

#include "Runtime/GfxDevice/GfxDeviceConfigure.h"
#include "ApiGLES.h"
#include "DeviceStateGLES.h"
#include "VertexDeclarationGLES.h"
#include "Runtime/GfxDevice/GfxDevice.h"

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

    GFX_API void	SetUserBackfaceMode(bool enable);
    GFX_API void	SetForceCullMode(CullMode mode);
    GFX_API void	SetWireframe(bool wire);
    GFX_API bool	GetWireframe() const;
    GFX_API void	SetInvertProjectionMatrix(bool enable);

    GFX_API void	SetWorldMatrix(const Matrix4x4f& matrix);
    GFX_API void	SetViewMatrix(const Matrix4x4f& matrix);
    GFX_API void	SetProjectionMatrix(const Matrix4x4f& matrix);

    GFX_API void	SetBackfaceMode(bool backface);
    GFX_API void	SetViewport(const RectInt& rect);
    GFX_API RectInt	GetViewport() const;

    GFX_API GfxDeviceLevelGL GetDeviceLevel() const;

    GFX_API void	InvalidateState();


    GFX_API void	Flush();
    GFX_API void	FinishRendering();


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

    GFX_API void	AcquireThreadOwnership();
    GFX_API void	ReleaseThreadOwnership();

    void MemoryBarrierBeforeDraw(BarrierTime previousWriteTime, gl::MemoryBarrierType type);
    void MemoryBarrierImmediate(BarrierTime previousWriteTime, gl::MemoryBarrierType type);
    void SetBarrierMask(GLbitfield mask);
    void DoMemoryBarriers();

    GFX_API bool HasActiveRandomWriteTarget() const;

    GFX_API void DrawNullGeometry(GfxPrimitiveType topology, int vertexCount, int instanceCount);
    GFX_API void SetAntiAliasFlag(bool aa);

    // This will copy whole platform render surface descriptor (and not just store argument pointer)

    // Temporary hack, really m_State should be a member of GfxContextGLES
    DeviceStateGLES& State() { return m_State; }

    GfxFramebufferGLES& GetFramebuffer();

protected:

    GFX_API DynamicVBO*	CreateDynamicVBO();

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
}
