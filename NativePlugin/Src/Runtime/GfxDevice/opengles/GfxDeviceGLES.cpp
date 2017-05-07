#include "PluginPrefix.h"

#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "GfxDeviceGLES.h"
#include "ApiConstantsGLES.h"
#include "AssertGLES.h"
#include "DeviceStateGLES.h"
#include "GraphicsCapsGLES.h"
#include "VertexDeclarationGLES.h"
#include "FrameBufferGLES.h"
#include "GfxContextGLES.h"

#include "Runtime/Math/Matrix4x4.h"
#include "Runtime/Math/Matrix3x3.h"

#include "Runtime/Shaders/GraphicsCaps.h"

#include "Runtime/GfxDevice/ChannelAssigns.h"
#include "Runtime/GfxDevice/opengles/GfxDeviceResourcesGLES.h"

#include "Runtime/Graphics/Mesh/GenericDynamicVBO.h"


#if UNITY_ANDROID
#include "PlatformDependent/AndroidPlayer/Source/EntryPoint.h"
#include "PlatformDependent/AndroidPlayer/Source/AndroidSystemInfo.h"
#endif

#define UNITY_DESKTOP 0

extern GfxDeviceLevelGL g_RequestedGLLevel;
const char* GetGfxDeviceLevelString(GfxDeviceLevelGL deviceLevel);

namespace
{
    void SetDepthState(ApiGLES & api, DeviceStateGLES& state, const DeviceDepthStateGLES* newState);
    void SetBlendState(ApiGLES & api, DeviceStateGLES& state, const DeviceBlendStateGLES* newState);
    void SetRasterState(ApiGLES & api, DeviceStateGLES& state, const DeviceRasterStateGLES* newState);
}//namespace


GfxDeviceGLES::GfxDeviceGLES()
{
}

GfxDeviceGLES::~GfxDeviceGLES()
{
    DeleteDynamicVBO();
    ReleaseBufferManagerGLES();
    vertDeclCache.Clear();
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
    g_RequestedGLLevel = deviceLevel;

    // Initialize context and states
    g_DeviceStateGLES = &m_State;

    if (IsGfxLevelES2(deviceLevel))
        m_Renderer = kGfxRendererOpenGLES20Mobile;
    else if (IsGfxLevelES(deviceLevel))
        m_Renderer = kGfxRendererOpenGLES30;
    else if (IsGfxLevelCore(deviceLevel))
        m_Renderer = kGfxRendererOpenGLCore;
    else
        GLES_ASSERT(&m_Api, 0, "OPENGL ERROR: Invalid device level");

    m_Api.Init(deviceLevel);
    gGL = m_State.api = &m_Api;

    m_AtomicCounterBuffer = NULL;
    for (int i = 0; i < kMaxAtomicCounters; i++)
        m_AtomicCounterSlots[i] = NULL;

    return true;
}

GfxDeviceLevelGL GfxDeviceGLES::GetDeviceLevel() const
{
    return GetGraphicsCaps().gles.featureLevel;
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
    ::SetBlendState(m_Api, m_State, newState);
}

void GfxDeviceGLES::SetDepthState(const DeviceDepthState* state)
{
    const DeviceDepthStateGLES* newState = (const DeviceDepthStateGLES*)state;
    ::SetDepthState(m_Api, m_State, newState);
}

void GfxDeviceGLES::SetRasterState(const DeviceRasterState* state)
{
    const DeviceRasterStateGLES* newState = (const DeviceRasterStateGLES*)state;
    ::SetRasterState(m_Api, m_State, newState);
}

void GfxDeviceGLES::SetStencilState(const DeviceStencilState* state, int stencilRef)
{
    // todo
}

void GfxDeviceGLES::SetInvertProjectionMatrix(bool enable)
{
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

// Acquire thread ownership on the calling thread. Worker releases ownership.
void GfxDeviceGLES::AcquireThreadOwnership()
{
}

// Release thread ownership on the calling thread. Worker acquires ownership.
void GfxDeviceGLES::ReleaseThreadOwnership()
{
    // Flush to make sure that commands enter gpu in correct order.
    // For example glTexSubImage can cause issues without flushing.
    m_Api.Submit();
}

void GfxDeviceGLES::Flush()
{
    m_Api.Submit(gl::SUBMIT_FLUSH);
}

void GfxDeviceGLES::FinishRendering()
{
    m_Api.Submit(gl::SUBMIT_FINISH);
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


void GfxDeviceGLES::BeforeDrawCall()
{
    // todo
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
    }

    if (indexBuffer)
        indexBuffer->RecordRender();

    for (int i = 0; i < vertexStreamCount; i++)
    {
        if (vertexStreams[i].buffer)
            ((VertexBufferGLES*)vertexStreams[i].buffer)->RecordRender();
    }
}

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
        if (m_State.requiredBarriers & m_State.requiredBarriersMask)
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




{
    return m_State.randomWriteTargetMaxIndex != -1;
}

struct ComputeProgramGLES
{
    GLuint program;
};






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
    { NULL,					ProgramUniformMatrix2fv,	ProgramUniformMatrix2x3fv,	ProgramUniformMatrix2x4fv },
    { NULL,					ProgramUniformMatrix3x2fv,	ProgramUniformMatrix3fv,	ProgramUniformMatrix3x4fv },
    { NULL,					ProgramUniformMatrix4x2fv,	ProgramUniformMatrix4x3fv,	ProgramUniformMatrix4fv }
};

// ProgramUniform*(u)iv funcs
ProgramUniformFunc intUniformFuncs[4] = { ProgramUniform1iv, ProgramUniform2iv, ProgramUniform3iv, ProgramUniform4iv };
ProgramUniformFunc uintUniformFuncs[4] = { ProgramUniform1uiv, ProgramUniform2uiv, ProgramUniform3uiv, ProgramUniform4uiv };


void GfxDeviceGLES::DrawNullGeometry(GfxPrimitiveType topology, int vertexCount, int instanceCount)
{
    GLES_ASSERT(&m_Api, GetGraphicsCaps().hasInstancing, "Instancing is not supported");

    BeforeDrawCall();

    m_Api.DrawArrays(topology, 0, vertexCount, instanceCount);
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
        if (curState == newState)
            return;

        state.depthState = newState;
        GLES_ASSERT(gGL, state.depthState, "Invalid depth state object");

        const CompareFunction newDepthFunc = (CompareFunction)newState->sourceState.depthFunc;
        const CompareFunction curDepthFunc = (CompareFunction)curState->sourceState.depthFunc;

        if (curDepthFunc != newDepthFunc)
        {
            if (newDepthFunc == kFuncDisabled)
            {
                api.Disable(gl::kDepthTest);
            }
            else
            {
                if (curDepthFunc == kFuncDisabled)
                    api.Enable(gl::kDepthTest);
                GLES_CALL(&api, glDepthFunc, newState->glFunc);
            }
        }

        const bool write = newState->sourceState.depthWrite;
        if (write != curState->sourceState.depthWrite)
            GLES_CALL(&api, glDepthMask, write ? GL_TRUE : GL_FALSE);
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

        if (curState != newState)
        {
            const UInt32 newColorMask = newState->sourceState.renderTargetWriteMask;
            if (curState->sourceState.renderTargetWriteMask != newColorMask)
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
        if (curState == newState)
            return;

        state.rasterState = newState;
        GLES_ASSERT(gGL, state.rasterState, "Invalid raster state");

        CullMode cull = newState->sourceState.cullMode;
        if (cull != curState->sourceState.cullMode)
            api.SetCullMode(cull);

        float zFactor = newState->sourceState.slopeScaledDepthBias;
        float zUnits = newState->sourceState.depthBias;
        if (zFactor != curState->sourceState.slopeScaledDepthBias || zUnits != curState->sourceState.depthBias)
        {
#if UNITY_ANDROID
            // Compensate for PolygonOffset bug on old Mali and Pvr GPUs
            if (GetGraphicsCaps().gles.haspolygonOffsetBug)
            {
                zFactor *= 16.0F;
            }
#endif

            GLES_CALL(&api, glPolygonOffset, zFactor, zUnits);
            if (zFactor || zUnits)
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

    srcData.depthBias += depthBias;
    srcData.slopeScaledDepthBias += slopeDepthBias;

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

