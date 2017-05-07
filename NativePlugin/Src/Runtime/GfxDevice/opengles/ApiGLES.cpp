#include "PluginPrefix.h"

#include "ApiTranslateGLES.h"
#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "GfxContextGLES.h"
#include "DeviceStateGLES.h"

#include "Runtime/GfxDevice/opengles/AssertGLES.h"
#include "Runtime/Shaders/GraphicsCaps.h"
#include "Runtime/Utilities/ArrayUtility.h"
//#include "Runtime/Utilities/Argv.h"

#define GLES_DEBUG_CHECK_LOG 0

namespace
{
    const GLenum GL_VENDOR = 0x1F00;
    const GLenum GL_EXTENSIONS = 0x1F03;
    const GLenum GL_NUM_EXTENSIONS = 0x821D;

    // Debug
    const GLenum GL_DEBUG_SOURCE_APPLICATION = 0x824A;

    // Query enums
    const GLenum GL_SAMPLES = 0x80A9;
    const GLenum GL_READ_BUFFER = 0x0C02;

    // Framebuffer enums
    const GLenum GL_SCALED_RESOLVE_NICEST_EXT = 0x90BB;
    const GLenum GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE = 0x8CD0;
    const GLenum GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME = 0x8CD1;

    // Polygon mode
    const GLenum GL_LINE = 0x1B01;
    const GLenum GL_FILL = 0x1B02;

    const GLenum GL_TEXTURE = 0x1702;

    const GLenum GL_PATCHES = 0x000E;

    const GLenum GL_INFO_LOG_LENGTH = 0x8B84;
    const GLenum GL_COMPILE_STATUS = 0x8B81;
    const GLenum GL_SHADER_SOURCE_LENGTH = 0x8B88;
    const GLenum GL_SHADER_TYPE = 0x8B4F;

    const GLenum GL_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE;
    const GLenum GL_TEXTURE_LOD_BIAS = 0x8501;
    const GLenum GL_TEXTURE_BASE_LEVEL = 0x813C;

    const GLenum GL_TEXTURE0 = 0x84C0;
    const GLenum GL_TEXTURE_SWIZZLE_R = 0x8E42;
    const GLenum GL_TEXTURE_SWIZZLE_G = 0x8E43;
    const GLenum GL_TEXTURE_SWIZZLE_B = 0x8E44;
    const GLenum GL_TEXTURE_SWIZZLE_A = 0x8E45;
    const GLenum GL_TEXTURE_SWIZZLE_RGBA = 0x8E46;

    const GLenum GL_FRONT_AND_BACK = 0x0408;

    const GLenum GL_PATCH_VERTICES = 0x8E72;

    const GLenum GL_UNPACK_ROW_LENGTH = 0x0CF2;
    const GLenum GL_PACK_ALIGNMENT = 0x0D05;
    const GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;

    const GLenum GL_RED_BITS = 0x0D52;
    const GLenum GL_GREEN_BITS = 0x0D53;
    const GLenum GL_BLUE_BITS = 0x0D54;
    const GLenum GL_ALPHA_BITS = 0x0D55;
    const GLenum GL_DEPTH_BITS = 0x0D56;
    const GLenum GL_STENCIL_BITS = 0x0D57;
    const GLenum GL_COVERAGE_BUFFERS_NV = 0x8ED3;
    const GLenum GL_COVERAGE_SAMPLES_NV = 0x8ED4;

    const GLenum GL_TEXTURE_SPARSE_ARB = 0x91A6;
    const GLenum GL_NUM_SPARSE_LEVELS_ARB = 0x91AA;
    const GLenum GL_VIRTUAL_PAGE_SIZE_X_ARB = 0x9195;
    const GLenum GL_VIRTUAL_PAGE_SIZE_Y_ARB = 0x9196;

    const GLenum GL_R32UI = 0x8236;
    const GLenum GL_TEXTURE_SRGB_DECODE_EXT = 0x8A48;

    const char* GetDebugFramebufferAttachmentType(GLint type)
    {
        switch (type)
        {
        case GL_RENDERBUFFER:	return "GL_RENDERBUFFER";
        case GL_TEXTURE:		return "GL_TEXTURE";
        default:				return "GL_NONE";
        }
    }

}//namespace

namespace gles
{
    void InitCaps(ApiGLES * api, GraphicsCaps* caps, GfxDeviceLevelGL &level);
    void InitRenderTextureFormatSupport(ApiGLES * api, GraphicsCaps* caps);
}//namespace gles

namespace {
    namespace BuggyBindElementArrayBufferWorkaround {

        // Vivante GC1000 driver (Samsung Galaxy Tab 3 7.0 lite) sometimes(!) changes GL_ARRAY_BUFFER_BINDING when calling glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer).
        // To workaround this, we replace glBindBuffer with a wrapper that reverses this unwanted modification of GL_ARRAY_BUFFER_BINDING.

        static GLuint s_ArrayBufferBinding;
        static gl::BindBufferFunc s_OriginalBindBuffer;

        void GLES_APIENTRY BindBufferWrapper(const GLenum target, const GLuint buffer)
        {
            s_OriginalBindBuffer(target, buffer);

            if (target == GL_ELEMENT_ARRAY_BUFFER)
                s_OriginalBindBuffer(GL_ARRAY_BUFFER, s_ArrayBufferBinding); // undo possible unexpected changes
            else if (target == GL_ARRAY_BUFFER)
                s_ArrayBufferBinding = buffer; // keep track of current GL_ARRAY_BUFFER
        }

        void ResetTrackedState()
        {
            if (s_OriginalBindBuffer)
            {
                s_ArrayBufferBinding = 0;
            }
        }

        void InstallBindBufferWrapper(gl::BindBufferFunc& bindBuffer)
        {
            // install glBindBuffer shim
            if (bindBuffer != s_OriginalBindBuffer)
            {
                s_OriginalBindBuffer = bindBuffer;
                bindBuffer = &BindBufferWrapper;
            }
            ResetTrackedState();
        }

    } // namespace BuggyBindElementArrayBufferWorkaround
} // namespace

ApiGLES * gGL = NULL;
const GLuint ApiGLES::kInvalidValue = static_cast<GLuint>(-1);

ApiGLES::ApiGLES()
    : ApiFuncGLES()
    , m_Translate(new TranslateGLES)
    , translate(*m_Translate)
    , m_CurrentProgramBinding(0)
    , m_CurrentProgramHasTessellation(false)
    , m_CurrentVertexArrayBinding()
    , m_DefaultVertexArrayName()
    , m_CurrentDefaultVertexArrayEnabled(0)
    , m_CurrentCullMode(kCullOff)
    , m_CurrentPatchVertices(0)
    , m_CurrentCapEnabled(0)
    , m_CurrentPolygonModeWire(false)
    , m_CurrentTextureUnit(0)
#	if SUPPORT_THREADS
    , m_Thread(Thread::GetCurrentThreadID())
#	endif
    , m_Caching(false)
{
    this->m_CurrentSamplerBindings.fill(0);
    this->m_CurrentFramebufferBindings.fill(gl::FramebufferHandle());

    this->m_CurrentBufferBindings.fill(0);
    this->m_CurrentUniformBufferBindings.fill(0);
    this->m_CurrentTransformBufferBindings.fill(0);
    this->m_CurrentStorageBufferBindings.fill(0);
    this->m_CurrentAtomicCounterBufferBindings.fill(0);
}

ApiGLES::~ApiGLES()
{
    delete m_Translate;
    m_Translate = NULL;
}

void ApiGLES::Init(GfxDeviceLevelGL &deviceLevel)
{
    // Set the current API
    gGL = this;

    // Caps are init by InitCaps but we need to initialize the featureLevel before calling Load which load OpenGL functions because it checks for extensions using the featureLevel
    //Assert(GetGraphicsCaps().gles.featureLevel == kGfxLevelUninitialized);
    GetGraphicsCaps().gles.featureLevel = deviceLevel;

    // The order of these functions matters

    // Load the OpenGL API pointers
    this->Load(deviceLevel);

    // Initialize the capabilities supported by the current platform
    gles::InitCaps(this, &GetGraphicsCaps(), deviceLevel);

    // Initialize the translation to OpenGL enums depending on the platform capability
    this->m_Translate->Init(GetGraphicsCaps(), deviceLevel);

    if (GetGraphicsCaps().gles.buggyBindElementArrayBuffer)
        BuggyBindElementArrayBufferWorkaround::InstallBindBufferWrapper(this->glBindBuffer);

    // Try to create framebuffer objects to figure out the effective support texture formats
    gles::InitRenderTextureFormatSupport(this, &GetGraphicsCaps());

#	if UNITY_ANDROID
    UpdateTextureFormatSupportETC2(this, deviceLevel);
#	endif
}

void ApiGLES::Dispatch(UInt32 threadGroupsX, UInt32 threadGroupsY, UInt32 threadGroupsZ)
{
    //GLES_CHECK(this, -1);

    GLES_CALL(this, glDispatchCompute, threadGroupsX, threadGroupsY, threadGroupsZ);
}

void ApiGLES::DispatchIndirect(UInt32 indirect)
{
    //GLES_CHECK(this, -1);

    GLES_CALL(this, glDispatchComputeIndirect, (GLintptr)indirect);
}

void ApiGLES::DrawArrays(GfxPrimitiveType topology, UInt32 firstVertex, UInt32 vertexCount, UInt32 instanceCount)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, topology >= kPrimitiveTypeFirst && topology <= kPrimitiveTypeLast, "Invalid 'topology' value");
    GLES_ASSERT(this, (GetGraphicsCaps().hasInstancing && instanceCount > 1) || instanceCount <= 1, "Requested an instanced draw but instanced draws are not supported");
    GLES_ASSERT(this, this->debug.ProgramStatus(), "The current state can't execute the bound GLSL program");

    const GLenum translatedTopology = m_CurrentProgramHasTessellation ? GL_PATCHES : this->translate.Topology(topology);

    if (GetGraphicsCaps().hasInstancing && instanceCount > 1)
        GLES_CALL(this, glDrawArraysInstanced, translatedTopology, firstVertex, vertexCount, instanceCount);
    else
        GLES_CALL(this, glDrawArrays, translatedTopology, firstVertex, vertexCount);
}

void ApiGLES::DrawElements(GfxPrimitiveType topology, const void * indicesOrOffset, UInt32 indexCount, UInt32 baseVertex, UInt32 instanceCount)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, topology >= kPrimitiveTypeFirst && topology <= kPrimitiveTypeLast, "Invalid 'topology' value");
    GLES_ASSERT(this, (GetGraphicsCaps().hasInstancing && instanceCount > 1) || instanceCount <= 1, "Requested an instanced draw but instanced draws are not supported");
    GLES_ASSERT(this, (GetGraphicsCaps().gles.hasDrawBaseVertex && baseVertex > 0) || baseVertex <= 0, "Requested a base vertex draw but it is not supported");
    GLES_ASSERT(this, this->debug.ProgramStatus(), "The current state can't execute the bound GLSL program");

    const GLenum translatedTopology = m_CurrentProgramHasTessellation ? GL_PATCHES : this->translate.Topology(topology);

    if (GetGraphicsCaps().gles.hasDrawBaseVertex && baseVertex > 0)
    {
        if (GetGraphicsCaps().hasInstancing && instanceCount > 1)
            GLES_CALL(this, glDrawElementsInstancedBaseVertex, translatedTopology, indexCount, GL_UNSIGNED_SHORT, indicesOrOffset, instanceCount, baseVertex);
        else
            GLES_CALL(this, glDrawElementsBaseVertex, translatedTopology, indexCount, GL_UNSIGNED_SHORT, indicesOrOffset, baseVertex);
    }
    else if (GetGraphicsCaps().hasInstancing && instanceCount > 1)
        GLES_CALL(this, glDrawElementsInstanced, translatedTopology, indexCount, GL_UNSIGNED_SHORT, indicesOrOffset, instanceCount);
    else
        GLES_CALL(this, glDrawElements, translatedTopology, indexCount, GL_UNSIGNED_SHORT, indicesOrOffset);
}

void ApiGLES::DrawCapture(GfxPrimitiveType topology, UInt32 VertexCount)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, topology >= kPrimitiveTypeFirst && topology <= kPrimitiveTypeLast, "Invalid 'topology' value");
    GLES_ASSERT(this, this->debug.ProgramStatus(), "The current state can't execute the bound GLSL program");

    GLES_CALL(this, glBeginTransformFeedback, this->translate.Topology(topology));

    this->DrawArrays(topology, 0, VertexCount, 1);

    GLES_CALL(this, glEndTransformFeedback);
}

void ApiGLES::DrawIndirect(GfxPrimitiveType topology, UInt32 bufferOffset)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, topology >= kPrimitiveTypeFirst && topology <= kPrimitiveTypeLast, "Invalid 'DrawParam' topology");
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasIndirectDraw, "Indirect draw is not supported");
    GLES_ASSERT(this, this->debug.ProgramStatus(), "The current state can't execute the bound GLSL program");

    GLES_CALL(this, glDrawArraysIndirect, this->translate.Topology(topology), (void*)(intptr_t)bufferOffset);
}

void ApiGLES::Clear(GLbitfield flags, const ColorRGBAf& color, bool onlyColorAlpha, float depth, int stencil)
{
    GLES_CHECK(this, -1);

    if (flags == 0)
        return;

    if (onlyColorAlpha)
        GLES_CALL(this, glColorMask, false, false, false, true);

    if (flags & GL_COLOR_BUFFER_BIT)
        GLES_CALL(this, glClearColor, color.r, color.g, color.b, color.a);

    if (flags & GL_DEPTH_BUFFER_BIT)
    {
        if (GetGraphicsCaps().gles.hasClearBufferFloat)
            GLES_CALL(this, glClearDepthf, depth);
        else
            GLES_CALL(this, glClearDepth, depth);
    }

    if (flags & GL_STENCIL_BUFFER_BIT)
        GLES_CALL(this, glClearStencil, stencil);

    GLES_CALL(this, glClear, flags);

    if (onlyColorAlpha)
        GLES_CALL(this, glColorMask, true, true, true, true);
}

GLuint ApiGLES::CreateShader(gl::ShaderStage stage, const char* source)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, source, "'source' is null, pass a valid GLSL shader source");

    GLuint shaderName = 0;
    GLES_CALL_RET(this, shaderName, glCreateShader, this->translate.GetShaderStage(stage));
    GLES_ASSERT(this, shaderName, "Shader object failed to be created");

    GLES_CALL(this, glShaderSource, shaderName, 1, &source, NULL);

    // We compile but we don't wait to the return allowing the drivers to thread the compilation
    GLES_CALL(this, glCompileShader, shaderName);

    GLES_CHECK(this, shaderName);
    return shaderName;
}

void ApiGLES::LinkProgram(GLuint programName)
{
    GLES_CHECK(this, programName);
    GLES_ASSERT(this, programName > 0, "'programName' is not a valid program object");

    GLES_CALL(this, glLinkProgram, programName);
}

bool ApiGLES::CheckProgram(GLuint & programName)
{
    GLES_CHECK(this, programName);
    GLES_ASSERT(this, programName > 0, "'program' is not a valid program object");

    GLint status = 0;
    GLES_CALL(this, glGetProgramiv, programName, GL_LINK_STATUS, &status);
    if (status == GL_TRUE)
        return true; // Success!

    // An compilation error happened
    GLint infoLogLength = 0;
    GLES_CALL(this, glGetProgramiv, programName, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength)
    {
        std::vector<char> infoLogBuffer(infoLogLength);
        GLES_CALL(this, glGetProgramInfoLog, programName, infoLogLength, NULL, &infoLogBuffer[0]);
    }

    this->DeleteProgram(programName);

    return false; // Link error
}

GLuint ApiGLES::CreateProgram()
{
    GLES_CHECK(this, -1);

    GLuint programName = 0;
    GLES_CALL_RET(this, programName, glCreateProgram);

    GLES_CHECK(this, programName);
    return programName;
}

GLuint ApiGLES::CreateGraphicsProgram(GLuint vertexShader, GLuint controlShader, GLuint evaluationShader, GLuint geometryShader, GLuint fragmentShader)
{
    GLES_CHECK(this, -1);

    GLuint programName = 0;
    GLES_CALL_RET(this, programName, glCreateProgram);

    if (g_GraphicsCapsGLES->hasBinaryShaderRetrievableHint)
        GLES_CALL(this, glProgramParameteri, programName, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    if (vertexShader)
        GLES_CALL(this, glAttachShader, programName, vertexShader);
    if (controlShader)
        GLES_CALL(this, glAttachShader, programName, controlShader);
    if (evaluationShader)
        GLES_CALL(this, glAttachShader, programName, evaluationShader);
    if (geometryShader)
        GLES_CALL(this, glAttachShader, programName, geometryShader);
    if (fragmentShader)
        GLES_CALL(this, glAttachShader, programName, fragmentShader);

    GLES_CHECK(this, programName);
    return programName;
}

GLuint ApiGLES::CreateComputeProgram(GLuint shaderName)
{
    GLES_CHECK(this, -1);

    GLuint programName = 0;
    GLES_CALL_RET(this, programName, glCreateProgram);
    GLES_CALL(this, glAttachShader, programName, shaderName);
    this->LinkProgram(programName);

    GLES_CHECK(this, programName);
    return programName;
}

void ApiGLES::DeleteProgram(GLuint & programName)
{
    GLES_CHECK(this, programName);
    //	GLES_ASSERT(this, programName, "Trying to delete an invalid program object");

    if (!programName || programName == kInvalidValue)
        return;

    if (this->m_CurrentProgramBinding == programName)
        this->BindProgram(0);

    GLES_CALL(this, glDeleteProgram, programName);
    programName = kInvalidValue;
}

GLenum ApiGLES::BindMemoryBuffer(GLuint bufferName, gl::BufferTarget _target)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, bufferName, "'buffer' object hasn't being created already");

    // On WebGL and Mali, we need to always use the buffer target used when creating the buffer object.
    gl::BufferTarget target = GetGraphicsCaps().gles.useActualBufferTargetForUploads ? _target : GetGraphicsCaps().gles.memoryBufferTargetConst;
    const GLenum translatedTarget = this->translate.GetBufferTarget(target);

    if (m_Caching && this->m_CurrentBufferBindings[target] == bufferName)
        return translatedTarget;
    this->m_CurrentBufferBindings[target] = bufferName;

    GLES_CALL(this, glBindBuffer, translatedTarget, bufferName);

    return translatedTarget;
}

void ApiGLES::UnbindMemoryBuffer(gl::BufferTarget _target)
{
    GLES_CHECK(this, 0);
    GLES_ASSERT(this, GetGraphicsCaps().gles.buggyBindBuffer, "This function is just a wordaround code for drivers (iOS) and should not be called otherwise.");

    // On WebGL and Mali, we need to always use the buffer target used when creating the buffer object.
    gl::BufferTarget target = GetGraphicsCaps().gles.useActualBufferTargetForUploads ? _target : GetGraphicsCaps().gles.memoryBufferTargetConst;

    this->m_CurrentBufferBindings[target] = 0;
    GLES_CALL(this, glBindBuffer, this->translate.GetBufferTarget(target), 0);
}

void ApiGLES::BindReadBuffer(GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasBufferCopy, "Buffer copy not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (m_Caching && this->m_CurrentBufferBindings[gl::kCopyReadBuffer] == bufferName)
        return;

    this->m_CurrentBufferBindings[gl::kCopyReadBuffer] = bufferName;
    GLES_CALL(this, glBindBuffer, GL_COPY_READ_BUFFER, bufferName);
}

void ApiGLES::BindElementArrayBuffer(GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (m_Caching && this->m_CurrentBufferBindings[gl::kElementArrayBuffer] == bufferName)
        return;

#	if UNITY_WEBGL
    if (bufferName == 0) // Cannot set buffer to 0 on webgl
        return;
#	endif

    this->m_CurrentBufferBindings[gl::kElementArrayBuffer] = bufferName;
    GLES_CALL(this, glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, bufferName);

    // Carry on an old and uncommented craft from the previous API overload code... but do we need that?
    g_DeviceStateGLES->transformDirtyFlags |= TransformState::kWorldViewProjDirty;
}

void ApiGLES::BindArrayBuffer(GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (m_Caching && this->m_CurrentBufferBindings[gl::kArrayBuffer] == bufferName)
        return;

#	if UNITY_WEBGL
    if (bufferName == 0) // Cannot set buffer to 0 on webgl
        return;
#	endif

    this->m_CurrentBufferBindings[gl::kArrayBuffer] = bufferName;

    GLES_CALL(this, glBindBuffer, GL_ARRAY_BUFFER, bufferName);
}

void ApiGLES::BindDrawIndirectBuffer(GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (m_Caching && this->m_CurrentBufferBindings[gl::kDrawIndirectBuffer] == bufferName)
        return;

    this->m_CurrentBufferBindings[gl::kDrawIndirectBuffer] = bufferName;
    GLES_CALL(this, glBindBuffer, GL_DRAW_INDIRECT_BUFFER, bufferName);
}

void ApiGLES::BindDispatchIndirectBuffer(GLuint buffer)
{
    GLES_CHECK(this, buffer);

    if (m_Caching && this->m_CurrentBufferBindings[gl::kDispatchIndirectBuffer] == buffer)
        return;

    this->m_CurrentBufferBindings[gl::kDispatchIndirectBuffer] = buffer;
    GLES_CALL(this, glBindBuffer, GL_DISPATCH_INDIRECT_BUFFER, buffer);
}

void ApiGLES::BindUniformBuffer(GLuint index, GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, index < GetGraphicsCaps().gles.maxUniformBufferBindings, "Unsupported uniform buffer binding");

    if (m_Caching && this->m_CurrentUniformBufferBindings[index] == bufferName)
        return;

    this->m_CurrentUniformBufferBindings[index] = bufferName;
    GLES_CALL(this, glBindBufferBase, GL_UNIFORM_BUFFER, index, bufferName);
}

void ApiGLES::BindTransformFeedbackBuffer(GLuint index, GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, index < GetGraphicsCaps().gles.maxTransformFeedbackBufferBindings, "Unsupported transform feedback buffer binding");

    if (m_Caching && this->m_CurrentTransformBufferBindings[index] == bufferName)
        return;
    this->m_CurrentTransformBufferBindings[index] = bufferName;

    GLES_CALL(this, glBindBufferBase, GL_TRANSFORM_FEEDBACK_BUFFER, index, bufferName);
}

void ApiGLES::BindShaderStorageBuffer(GLuint index, GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, index < GetGraphicsCaps().gles.maxShaderStorageBufferBindings, "Unsupported shader storage buffer binding");

    if (m_Caching && this->m_CurrentStorageBufferBindings[index] == bufferName)
        return;
    this->m_CurrentStorageBufferBindings[index] = bufferName;

    GLES_CALL(this, glBindBufferBase, GL_SHADER_STORAGE_BUFFER, index, bufferName);
}

void ApiGLES::BindAtomicCounterBuffer(GLuint index, GLuint bufferName)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, index < GetGraphicsCaps().gles.maxAtomicCounterBufferBindings, "Unsupported atomic counter buffer binding");

    if (m_Caching && this->m_CurrentAtomicCounterBufferBindings[index] == bufferName)
        return;
    this->m_CurrentAtomicCounterBufferBindings[index] = bufferName;

    GLES_CALL(this, glBindBufferBase, GL_ATOMIC_COUNTER_BUFFER, index, bufferName);
}

GLuint ApiGLES::CreateBuffer(gl::BufferTarget target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, target >= gl::kBufferTargetFirst && target <= gl::kBufferTargetLast, "Invalid target parameter");

    GLuint bufferName = 0;

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
        GLES_CALL(this, glCreateBuffers, 1, &bufferName);
    else
        GLES_CALL(this, glGenBuffers, 1, &bufferName);

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL(this, glNamedBufferData, bufferName, size, data, usage);
    }
    else
    {
        const GLenum memoryTarget = this->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glBufferData, memoryTarget, size, data, usage);
    }

    GLES_CHECK(this, bufferName);
    return bufferName;
}

void ApiGLES::DeleteBuffer(GLuint & bufferName)
{
    GLES_CHECK(this, bufferName);

    if (!bufferName || bufferName == kInvalidValue)
        return;

    // Reset the buffer caching for cases where with delete a buffer which name get reallocated right after.
    if (this->m_CurrentBufferBindings[gl::kArrayBuffer] == bufferName)
        this->BindArrayBuffer(0);
    if (this->m_CurrentBufferBindings[gl::kElementArrayBuffer] == bufferName)
        this->BindElementArrayBuffer(0);
    if (GetGraphicsCaps().gles.hasBufferQuery && this->m_CurrentBufferBindings[gl::kQueryBuffer] == bufferName)
    {
        this->glBindBuffer(GL_QUERY_BUFFER, 0);
        this->m_CurrentBufferBindings[gl::kQueryBuffer] = 0;
    }
    if (GetGraphicsCaps().gles.hasIndirectParameter && this->m_CurrentBufferBindings[gl::kParameterBuffer] == bufferName)
    {
        this->glBindBuffer(GL_PARAMETER_BUFFER_ARB, 0);
        this->m_CurrentBufferBindings[gl::kParameterBuffer] = 0;
    }
    if (GetGraphicsCaps().gles.hasBufferCopy)
    {
        if (this->m_CurrentBufferBindings[gl::kCopyReadBuffer] == bufferName)
        {
            this->glBindBuffer(GL_COPY_READ_BUFFER, 0);
            this->m_CurrentBufferBindings[gl::kCopyReadBuffer] = 0;
        }
        if (this->m_CurrentBufferBindings[gl::kCopyWriteBuffer] == bufferName)
        {
            this->glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
            this->m_CurrentBufferBindings[gl::kCopyWriteBuffer] = 0;
        }
    }
    if (GetGraphicsCaps().gles.hasIndirectDraw && this->m_CurrentBufferBindings[gl::kDrawIndirectBuffer] == bufferName)
    {
        this->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        this->m_CurrentBufferBindings[gl::kDrawIndirectBuffer] = 0;
    }
    for (std::size_t i = 0; i < m_CurrentUniformBufferBindings.size(); ++i)
    {
        if (m_CurrentUniformBufferBindings[i] != bufferName)
            continue;
        this->BindUniformBuffer(i, 0);
    }
    for (std::size_t i = 0; i < m_CurrentTransformBufferBindings.size(); ++i)
    {
        if (m_CurrentTransformBufferBindings[i] != bufferName)
            continue;
        this->BindTransformFeedbackBuffer(i, 0);
    }
    for (std::size_t i = 0; i < m_CurrentStorageBufferBindings.size(); ++i)
    {
        if (m_CurrentStorageBufferBindings[i] != bufferName)
            continue;
        this->BindShaderStorageBuffer(i, 0);
    }
    for (std::size_t i = 0; i < m_CurrentAtomicCounterBufferBindings.size(); ++i)
    {
        if (m_CurrentAtomicCounterBufferBindings[i] != bufferName)
            continue;
        this->BindAtomicCounterBuffer(i, 0);
    }

    // Now we can delete the buffer object
    GLES_CALL(this, glDeleteBuffers, 1, &bufferName);
    bufferName = kInvalidValue;
}

GLuint ApiGLES::RecreateBuffer(GLuint bufferName, gl::BufferTarget target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, bufferName, "'buffer' object hasn't being created already");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL(this, glNamedBufferData, bufferName, size, data, usage);
    }
    else
    {
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glBufferData, memoryTarget, size, data, usage);

        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }

    return bufferName;
}

void ApiGLES::UploadBufferSubData(GLuint bufferName, gl::BufferTarget target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, bufferName, "'buffer' object hasn't being created already");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL(this, glNamedBufferSubData, bufferName, offset, size, data);
    }
    else
    {
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glBufferSubData, memoryTarget, offset, size, data);

        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }
}

void* ApiGLES::MapBuffer(GLuint bufferName, gl::BufferTarget target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasMapbufferRange, "Map buffer range is not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    void* ptr = NULL;

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL_RET(this, ptr, glMapNamedBufferRange, bufferName, offset, length, access);
    }
    else
    {
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL_RET(this, ptr, glMapBufferRange, memoryTarget, offset, length, access);

        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }

    return ptr;
}

void ApiGLES::UnmapBuffer(GLuint bufferName, gl::BufferTarget target)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasMapbufferRange, "Map buffer range is not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL(this, glUnmapNamedBuffer, bufferName);
    }
    else
    {
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glUnmapBuffer, memoryTarget);

        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }
}

void ApiGLES::FlushBuffer(GLuint bufferName, gl::BufferTarget target, GLintptr offset, GLsizeiptr length)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasMapbufferRange, "Map buffer range is not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL(this, glFlushMappedNamedBufferRange, bufferName, offset, length);
    }
    else
    {
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glFlushMappedBufferRange, memoryTarget, offset, length);

        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }
}

void ApiGLES::CopyBufferSubData(GLuint srcBuffer, GLuint dstBuffer, GLintptr srcOffset, GLintptr dstOffset, GLsizeiptr size)
{
    GLES_CHECK(this, dstBuffer);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasBufferCopy, "Buffer copy is not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    gGL->BindMemoryBuffer(dstBuffer, gl::kCopyWriteBuffer);
    gGL->BindReadBuffer(srcBuffer);
    GLES_CALL(this, glCopyBufferSubData, GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, dstOffset, size);
}

void ApiGLES::ClearBuffer(GLuint bufferName, gl::BufferTarget target)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasBufferClear, "Buffer clear is not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLuint zeroData = 0;
        GLES_CALL(this, glClearNamedBufferData, bufferName, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zeroData);
    }
    else
    {
        GLuint zeroData = 0;
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glClearBufferData, memoryTarget, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zeroData);

        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }
}

void ApiGLES::ClearBufferSubData(GLuint bufferName, gl::BufferTarget target, GLintptr offset, GLsizeiptr size)
{
    GLES_CHECK(this, bufferName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasMapbufferRange, "buffer mapping is not supported");
    GLES_ASSERT(this, this->debug.BufferBindings(), "The context has been modified outside of ApiGLES. States tracking is lost.");

    GLuint zeroData = 0;

    if (CAN_HAVE_DIRECT_STATE_ACCESS && GetGraphicsCaps().gles.hasDirectStateAccess)
    {
        GLES_CALL(this, glClearNamedBufferSubData, bufferName, GL_R32UI, offset, size, GL_RED, GL_UNSIGNED_INT, &zeroData);
    }
    else if (GetGraphicsCaps().gles.hasBufferClear)
    {
        const GLenum memoryTarget = gGL->BindMemoryBuffer(bufferName, target);
        GLES_CALL(this, glClearBufferSubData, memoryTarget, GL_R32UI, offset, size, GL_RED, GL_UNSIGNED_INT, &zeroData);
        if (GetGraphicsCaps().gles.buggyBindBuffer)
            gGL->UnbindMemoryBuffer(target);
    }
    else if (GetGraphicsCaps().gles.hasMapbufferRange)
    {
        UInt32* pointer = static_cast<UInt32*>(this->MapBuffer(bufferName, target, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
        for (std::size_t i = 0, n = (size - offset) / sizeof(UInt32); i < n; ++i)
            *(pointer + i) = zeroData;
        this->UnmapBuffer(bufferName, target);
    }
    else
    {
        GLES_ASSERT(this, 0, "Unsupported GfxDeviceLevel");
    }
}

gl::VertexArrayHandle ApiGLES::CreateVertexArray()
{
    //GLES_ASSERT(this, GetGraphicsCaps().gles.hasVertexArrayObject, "Vertex array objects are not supported");

    GLuint VertexArrayName = 0;
    GLES_CALL(this, glGenVertexArrays, 1, &VertexArrayName);

    return gl::VertexArrayHandle(VertexArrayName);
}

void ApiGLES::DeleteVertexArray(gl::VertexArrayHandle& vertexArrayName)
{
    GLES_CHECK(this, vertexArrayName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasVertexArrayObject, "Vertex array objects are not supported");

    GLuint vaName = GLES_OBJECT_NAME(vertexArrayName);
    GLES_CALL(this, glDeleteVertexArrays, 1, &vaName);
    vertexArrayName = gl::VertexArrayHandle();
}

void ApiGLES::BindVertexArray(gl::VertexArrayHandle vertexArrayName)
{
    GLES_CHECK(this, vertexArrayName);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasVertexArrayObject, "Vertex Array Objects are not supported");

    if (m_Caching && this->m_CurrentVertexArrayBinding == vertexArrayName)
        return;
    this->m_CurrentVertexArrayBinding = vertexArrayName;

    GLES_CALL(this, glBindVertexArray, GLES_OBJECT_NAME(vertexArrayName));
}

bool ApiGLES::IsVertexArray(gl::VertexArrayHandle vertexArrayName)
{
    GLboolean isVertexArray = 0;
    GLES_CHECK(this, vertexArrayName);
    GLES_CALL_RET(this, isVertexArray, glIsVertexArray, GLES_OBJECT_NAME(vertexArrayName));
    return (0 != isVertexArray);
}

void ApiGLES::EnableVertexArrayAttrib(GLuint attribIndex, GLuint bufferName, gl::VertexArrayAttribKind Kind, GLint size, VertexChannelFormat format, GLsizei stride, const GLvoid* offset)
{
    GLES_CHECK(this, attribIndex);
    GLES_ASSERT(this, m_CurrentVertexArrayBinding == m_DefaultVertexArrayName, "VertexArrayAttribPointer can only be called on a default vertex array object by Unity design");
    GLES_ASSERT(this, attribIndex < GetGraphicsCaps().gles.maxAttributes, "Too many attributes used");
    GLES_ASSERT(this, size > 0 && size <= 4, "Invalid number of attribute components");

    // Enable vertex attribute
    if (!m_Caching || !(m_CurrentDefaultVertexArrayEnabled & (1 << attribIndex)))
    {
        GLES_CALL(this, glEnableVertexAttribArray, attribIndex);
        m_CurrentDefaultVertexArrayEnabled |= (1 << attribIndex);
    }

    // Setup the vertex attribute
    VertexArray vertexArray(*this, bufferName, size, format, stride, offset, Kind);
    if (m_Caching && CompareMemory(vertexArray, m_CurrentDefaultVertexArray[attribIndex]))
        return;
    m_CurrentDefaultVertexArray[attribIndex] = vertexArray;

    const GLenum type = this->translate.VertexType(format);

    GLES_ASSERT(this, bufferName != 0, "An array buffer must be bound to setup a vertex attribute");
    this->BindArrayBuffer(bufferName);

    switch (Kind)
    {
    case gl::kVertexArrayAttribSNorm:
    case gl::kVertexArrayAttribSNormNormalize:
        GLES_CALL(this, glVertexAttribPointer, attribIndex, size, type, Kind == gl::kVertexArrayAttribSNormNormalize ? GL_TRUE : GL_FALSE, stride, offset);
        break;
    case gl::kVertexArrayAttribInteger:
        GLES_CALL(this, glVertexAttribIPointer, attribIndex, size, type, stride, offset);
        break;
    case gl::kVertexArrayAttribLong:
        GLES_CALL(this, glVertexAttribLPointer, attribIndex, size, type, stride, offset);
        break;
    }
}

void ApiGLES::DisableVertexArrayAttrib(GLuint attribIndex)
{
    GLES_CHECK(this, attribIndex);
    GLES_ASSERT(this, m_CurrentVertexArrayBinding == m_DefaultVertexArrayName, "VertexArrayAttribPointer can only be called on a default vertex array object by Unity design");
    GLES_ASSERT(this, attribIndex < GetGraphicsCaps().gles.maxAttributes, "Too many attributes used");

    if (m_Caching && !(m_CurrentDefaultVertexArrayEnabled & (1 << attribIndex)))
        return;
    m_CurrentDefaultVertexArrayEnabled &= ~(1 << attribIndex);

    GLES_CALL(this, glDisableVertexAttribArray, attribIndex);
}

void ApiGLES::SetPatchVertices(int count)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, GetGraphicsCaps().gles.hasTessellationShader, "Tessellation is not supported");

    if (m_Caching && m_CurrentPatchVertices == count)
        return;
    m_CurrentPatchVertices = count;

    GLES_CALL(this, glPatchParameteri, GL_PATCH_VERTICES, static_cast<GLint>(count));
}

void ApiGLES::Enable(gl::EnabledCap cap)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, (cap == gl::kTextureCubeMapSeamless && GetGraphicsCaps().gles.hasSeamlessCubemapEnable) || (cap != gl::kTextureCubeMapSeamless), "OPENGL ERROR: gl::kTextureCubeMapSeamless is not supported");

    const UInt64 flag = (static_cast<UInt64>(1) << cap);
    if (m_Caching && m_CurrentCapEnabled & flag)
        return;
    m_CurrentCapEnabled |= flag;

    GLES_CALL(this, glEnable, this->translate.Enable(cap));
}

void ApiGLES::Disable(gl::EnabledCap cap)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, (cap == gl::kTextureCubeMapSeamless && GetGraphicsCaps().gles.hasSeamlessCubemapEnable) || (cap != gl::kTextureCubeMapSeamless), "OPENGL ERROR: gl::kTextureCubeMapSeamless is not supported");

    const UInt64 flag = (static_cast<UInt64>(1) << cap);
    if (m_Caching && !(m_CurrentCapEnabled & flag))
        return;
    m_CurrentCapEnabled &= ~flag;

    GLES_CALL(this, glDisable, this->translate.Enable(cap));
}

bool ApiGLES::IsEnabled(gl::EnabledCap cap) const
{
    return m_CurrentCapEnabled & (static_cast<UInt64>(1) << cap) ? true : false;
}

void ApiGLES::SetPolygonMode(bool wire)
{
    GLES_CHECK(this, -1);

    if (!GetGraphicsCaps().gles.hasWireframe)
        return;

    if (m_Caching && m_CurrentPolygonModeWire == wire)
        return;
    this->m_CurrentPolygonModeWire = wire;

    if (wire)
    {
        this->Enable(gl::kPolygonOffsetLine);
        GLES_CALL(this, glPolygonMode, GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        this->Disable(gl::kPolygonOffsetLine);
        GLES_CALL(this, glPolygonMode, GL_FRONT_AND_BACK, GL_FILL);
    }
}

void ApiGLES::SetCullMode(const ::CullMode cullMode)
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, cullMode != kCullUnknown, "Invalid cull mode");
    GLES_ASSERT(this, !m_Caching || this->debug.CullMode(), "OPENGL ERROR: The OpenGL context has been modified outside of ApiGLES. States tracking is lost.");

    if (m_Caching && m_CurrentCullMode == cullMode)
        return;
    this->m_CurrentCullMode = cullMode;

    switch (cullMode)
    {
    case kCullOff:
        this->Disable(gl::kCullFace);
        break;
    case kCullFront:
        GLES_CALL(this, glCullFace, GL_FRONT);
        this->Enable(gl::kCullFace);
        break;
    case kCullBack:
        GLES_CALL(this, glCullFace, GL_BACK);
        this->Enable(gl::kCullFace);
        break;
    default:
        GLES_ASSERT(this, 0, "Unkown cull mode");
    }
}

gl::QueryHandle ApiGLES::CreateQuery()
{
    GLES_CHECK(this, ApiGLES::kInvalidValue);
    GLES_ASSERT(this, GetGraphicsCaps().hasTimerQuery, "Timer query is not supported");

    GLuint queryName = 0;
    GLES_CALL(this, glGenQueries, 1, &queryName);

    GLES_CHECK(this, queryName);
    return gl::QueryHandle(queryName);
}

void ApiGLES::DeleteQuery(gl::QueryHandle & query)
{
    GLES_CHECK(this, GLES_OBJECT_NAME(query));
    GLES_ASSERT(this, GetGraphicsCaps().hasTimerQuery, "Timer query is not supported");

    GLuint const queryName = GLES_OBJECT_NAME(query);
    GLES_CALL(this, glDeleteQueries, 1, &queryName);
    query = gl::QueryHandle::kInvalidValue;
}

void ApiGLES::QueryTimeStamp(gl::QueryHandle query)
{
    GLES_CHECK(this, GLES_OBJECT_NAME(query));
    GLES_ASSERT(this, GetGraphicsCaps().hasTimerQuery, "Timer query is not supported");

    GLES_CALL(this, glQueryCounter, GLES_OBJECT_NAME(query), GL_TIMESTAMP);
}

GLuint64 ApiGLES::Query(gl::QueryHandle query, gl::QueryResult queryResult)
{
    GLES_CHECK(this, GLES_OBJECT_NAME(query));
    GLES_ASSERT(this, GetGraphicsCaps().hasTimerQuery, "Timer query is not supported");

    GLuint64 result = 0;
    GLES_CALL(this, glGetQueryObjectui64v, GLES_OBJECT_NAME(query), queryResult, &result);
    return result;
}

GLint ApiGLES::Get(GLenum cap) const
{
    GLint result = 0;
    GLES_CALL(this, glGetIntegerv, cap, &result);
    return result;
}

bool ApiGLES::QueryExtension(const char * extension) const
{
    GLES_CHECK(this, -1);

    // WebGL 2.0 does not seem to support GL_NUM_EXTENSIONS
    if (IsGfxLevelES2(GetGraphicsCaps().gles.featureLevel))
    {
        const char* extensions = reinterpret_cast<const char*>(this->glGetString(GL_EXTENSIONS));
        if (!extensions)
            return false;

        const char* match = strstr(extensions, extension);
        if (!match)
            return false;

        // we need an exact match, extensions string is a list of extensions separated by spaces, e.g. "GL_EXT_draw_buffers" should not match "GL_EXT_draw_buffers_indexed"
        const char* end = match + strlen(extension);
        return *end == ' ' || *end == '\0';
    }
    else
    {
        const GLint numExtensions = this->Get(GL_NUM_EXTENSIONS);
        for (GLint i = 0; i < numExtensions; ++i)
        {
            const char* Extension = reinterpret_cast<const char*>(this->glGetStringi(GL_EXTENSIONS, i));
            if (!strcmp(extension, Extension))
                return true;
        }
        return false;
    }
}

std::string ApiGLES::GetExtensionString() const
{
    GLES_CHECK(this, -1);

    std::string extensionString;

    // WebGL 2.0 does not seem to support GL_NUM_EXTENSIONS
    if (IsGfxLevelES2(GetGraphicsCaps().gles.featureLevel))
    {
        const GLubyte* string = 0;
        GLES_CALL_RET(this, string, glGetString, GL_EXTENSIONS);
        extensionString = reinterpret_cast<const char*>(string);
    }
    else
    {
        const GLint numExtensions = this->Get(GL_NUM_EXTENSIONS);
        for (GLint i = 0; i < numExtensions; ++i)
        {
            const GLubyte* string = 0;
            GLES_CALL_RET(this, string, glGetStringi, GL_EXTENSIONS, i);
            extensionString += std::string(" ") + reinterpret_cast<const char*>(string);
        }
    }

    return extensionString;
}

const char* ApiGLES::GetDriverString(gl::DriverQuery Query) const
{
    GLES_CHECK(this, -1);

    const GLenum translatedQuery = GL_VENDOR + Query; // GL_VENDOR, GL_RENDERER and GL_VERSION are contiguous values, query is a zero based enum
    const GLubyte* stringPointer = 0;
    GLES_CALL_RET(this, stringPointer, glGetString, translatedQuery);

    return reinterpret_cast<const char*>(stringPointer);
}

void ApiGLES::Submit(gl::SubmitMode mode)
{
    switch (mode)
    {
    case gl::SUBMIT_FINISH:
        GLES_CALL(this, glFinish);
        return;
    case gl::SUBMIT_FLUSH:
        GLES_CALL(this, glFlush);
        return;
    default:
        GLES_ASSERT(this, 0, "Unsupported submot mode");
    }
}

bool ApiGLES::Verify() const
{
    GLES_CHECK(this, -1);
    GLES_ASSERT(this, this->debug.Verify(), "The OpenGL context has been modified outside of ApiGLES. States tracking is lost.");

    typedef std::pair<bool, bool> Enable;

    struct IsEnabled
    {
        Enable operator()(const ApiGLES & api, const gl::EnabledCap & capUnity) const
        {
            return Enable(
                api.glIsEnabled(api.translate.Enable(capUnity)) == GL_TRUE,
                (api.m_CurrentCapEnabled & (static_cast<UInt64>(1) << capUnity)) != 0);
        }
    } isEnabled;

    bool check = true;

    if (GetGraphicsCaps().gles.hasDebugOutput)
    {
        const Enable & debugSync = isEnabled(*this, gl::kDebugOutputSynchronous);

        if (GetGraphicsCaps().gles.hasDebugKHR)
        {
            const Enable & debug = isEnabled(*this, gl::kDebugOutput);
        }
    }

    if (IsGfxLevelCore(GetGraphicsCaps().gles.featureLevel))
    {
        const Enable & colorLogicOp = isEnabled(*this, gl::kColorLogicOp);
        const Enable & depthClamp = isEnabled(*this, gl::kDepthClamp);
        const Enable & framebufferSRGB = isEnabled(*this, gl::kFramebufferSRGB);
        const Enable & lineSmooth = isEnabled(*this, gl::kLineSmooth);
        const Enable & multisample = isEnabled(*this, gl::kMultisample);
        const Enable & polygonOffsetLine = isEnabled(*this, gl::kPolygonOffsetLine);
        const Enable & polygonOffsetPoint = isEnabled(*this, gl::kPolygonOffsetpoint);
        const Enable & polygonSmooth = isEnabled(*this, gl::kPolygonSmooth);
        const Enable & primitiveRestart = isEnabled(*this, gl::kPrimitiveRestart);
        const Enable & sampleAlphaToOne = isEnabled(*this, gl::kSampleAlphaToOne);
        const Enable & sampleShading = isEnabled(*this, gl::kSampleShading);
        const Enable & textureCubeMapSeamless = isEnabled(*this, gl::kTextureCubeMapSeamless);
        const Enable & programPointSize = isEnabled(*this, gl::kProgramPointSize);
    }

    if (IsGfxLevelES(GetGraphicsCaps().gles.featureLevel, kGfxLevelES31) || IsGfxLevelCore(GetGraphicsCaps().gles.featureLevel, kGfxLevelCore40))
    {
        const Enable & sampleMask = isEnabled(*this, gl::kSampleMask);
    }

    const Enable & blend = isEnabled(*this, gl::kBlend);
    const Enable & cullFace = isEnabled(*this, gl::kCullFace);
    const Enable & depthTest = isEnabled(*this, gl::kDepthTest);
    const Enable & dither = isEnabled(*this, gl::kDither);
    const Enable & polygonOffsetFill = isEnabled(*this, gl::kPolygonOffsetFill);
    const Enable & sampleAlphaToCoverage = isEnabled(*this, gl::kSampleAlphaToCoverage);
    const Enable & sampleCoverage = isEnabled(*this, gl::kSampleCoverage);
    const Enable & scissorTest = isEnabled(*this, gl::kScissorTest);
    const Enable & stencilTest = isEnabled(*this, gl::kStencilTest);

    return check;
}

ApiGLES::VertexArray::VertexArray()
    : m_Offset(0)
    , m_Stride(0)
    , m_Buffer(0)
    , m_Bitfield(0)
{}

ApiGLES::VertexArray::VertexArray(const ApiGLES & api, GLuint buffer, GLint size, VertexChannelFormat format, GLsizei stride, const GLvoid* offset, gl::VertexArrayAttribKind kind)
    : m_Offset(offset)
    , m_Stride(stride)
    , m_Buffer(buffer)
    , m_Bitfield(0)
{
    this->m_Bitfield = api.translate.VertexArrayKindBitfield(kind) | api.translate.VertexArraySizeBitfield(size) | api.translate.VertexArrayTypeBitfield(format);
}

