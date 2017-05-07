#include "PluginPrefix.h"

#include "AssertGLES.h"
#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "GfxGetProcAddressGLES.h"
#include "GfxDeviceGLES.h"

//#include "Runtime/Misc/CPUInfo.h"
#include "Runtime/Shaders/GraphicsCaps.h"
#include "Runtime/Utilities/ArrayUtility.h"
#include "Runtime/GfxDevice/opengles/GfxDeviceResourcesGLES.h"

#if UNITY_ANDROID
#include "PlatformDependent/AndroidPlayer/Source/AndroidSystemInfo.h"
#endif

#if UNITY_IPHONE
extern "C" int UnityDeviceGeneration();
#endif

#if UNITY_OSX
#include <OpenGL/OpenGL.h>
#endif

#include <cctype>
#include <cstring>
#include <string>

#if defined(UNITY_WIN) || defined(UNITY_OSX)  
#define UNITY_DESKTOP 1
#else
#define UNITY_DESKTOP 0 
#endif

GraphicsCapsGLES* g_GraphicsCapsGLES = 0;

namespace systeminfo { int GetPhysicalMemoryMB(); }


#if UNITY_OSX

// Adapted from https://developer.apple.com/library/mac/qa/qa1168/_index.html
// "How do I determine how much VRAM is available on my video card?"
static int GetVideoMemoryMBOSX(int wantedRendererID)
{
    int vramMB = 0;

    const int kMaxDisplays = 8;
    CGDirectDisplayID displays[kMaxDisplays];
    CGDisplayCount displayCount;
    CGGetActiveDisplayList(kMaxDisplays, displays, &displayCount);
    CGOpenGLDisplayMask openGLDisplayMask = CGDisplayIDToOpenGLDisplayMask(displays[0]);

    CGLRendererInfoObj info;
    GLint numRenderers = 0;
    CGLError err = CGLQueryRendererInfo(openGLDisplayMask, &info, &numRenderers);
    if (0 == err)
    {
        for (int j = 0; j < numRenderers; ++j)
        {
            GLint rv = 0;
            // Get the accelerated one. Typically macs report 2 renderers, one actual GPU
            // and one software one. This is true even for dual-gpu systems like MBPs,
            // normally only the dGPU is reported, but if forcing the integrated one then
            // only the iGPU is reported as accelerated.
            err = CGLDescribeRenderer(info, j, kCGLRPAccelerated, &rv);
            if (rv == 1)
            {
                GLint thisRendererMB = 0;
                err = CGLDescribeRenderer(info, j, kCGLRPVideoMemoryMegabytes, &thisRendererMB);
                vramMB = thisRendererMB;
                break;
            }
        }
        CGLDestroyRendererInfo(info);
    }

    // Safeguards: all Macs support at least 128MB, but things like software device could
    // return zeroes.
    if (vramMB < 128)
        vramMB = 128;
    return vramMB;
}

#endif // if UNITY_OSX


// ---------------------------------------------------------------------------------------------
// Capability check per feature for each OpenGL renderer using the unified OpenGL back-end
// Those are pure functions only
namespace
{
    // List of the OpenGL capability queries
    const GLenum GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS = 0x92DC;
    const GLenum GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS = 0x90DD;
    const GLenum GL_MAX_TRANSFORM_FEEDBACK_BUFFERS = 0x8E70;
    const GLenum GL_MAX_UNIFORM_BUFFER_BINDINGS = 0x8A2F;
    const GLenum GL_MAX_VERTEX_UNIFORM_VECTORS = 0x8DFB;
    const GLenum GL_MAX_VARYING_VECTORS = 0x8DFC;
    const GLenum GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = 0x8B4D;
    const GLenum GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = 0x8B4C;
    const GLenum GL_MAX_TEXTURE_IMAGE_UNITS = 0x8872;
    const GLenum GL_MAX_FRAGMENT_UNIFORM_VECTORS = 0x8DFD;
    const GLenum GL_MAX_FRAGMENT_UNIFORM_COMPONENTS = 0x8B49;
    const GLenum GL_MAX_VERTEX_UNIFORM_COMPONENTS = 0x8B4A;
    const GLenum GL_MAX_TEXTURE_SIZE = 0x0D33;
    const GLenum GL_MAX_ARRAY_TEXTURE_LAYERS = 0x88FF;
    const GLenum GL_MAX_CUBE_MAP_TEXTURE_SIZE = 0x851C;
    const GLenum GL_MAX_UNIFORM_BLOCK_SIZE = 0x8A30;
    const GLenum GL_MAX_RENDERBUFFER_SIZE = 0x84E8;
    const GLenum GL_MAX_COLOR_TEXTURE_SAMPLES = 0x910E;
    const GLenum GL_MAX_SAMPLES_IMG = 0x9135;
    const GLenum GL_MAX_SAMPLES = 0x8D57;
    const GLenum GL_MAX_COLOR_ATTACHMENTS = 0x8CDF;
    const GLenum GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF;
    const GLenum GL_MAJOR_VERSION = 0x821B;
    const GLenum GL_MINOR_VERSION = 0x821C;

#if UNITY_OSX
    static UInt32 GetOSXVersion()
    {
        UInt32 response = 0;

        Gestalt(gestaltSystemVersion, (MacSInt32*)&response);
        return response;
    }
#endif

    bool RequireDrawBufferNone(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level)
    {
        // Support of GL_ARB_ES2_compatibility isn't right on some Intel HD 4000 and AMD 7670M drivers
        // Which requires to call glDrawBuffer(GL_NONE) on depth only framebuffer
        if (IsGfxLevelCore(level))
            return !api.QueryExtension("GL_ARB_ES2_compatibility") || caps.gles.isIntelGpu || caps.gles.isAMDGpu;
        return false;
    }

    bool HasDrawBuffers(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelCore(level) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (IsGfxLevelES2(level))
            return api.QueryExtension("WEBGL_draw_buffers") || (api.QueryExtension("GL_NV_draw_buffers") && api.QueryExtension("GL_NV_fbo_color_attachments"));
        return false;
    }

    bool HasES2Compatibility(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        // To run OpenGL ES shaders on OpenGL core context which is not supported yet and cause errors on OSX
        // because we generate OpenGL ES shaders that relies EXT_shadow_samplers which isn't support on desktop in the extension form.
#		if 1
        return false;
#		else
        return IsGfxLevelCore(level) ? api.QueryExtension("GL_ARB_ES2_compatibility") : false;
#		endif
    }

    bool HasES3Compatibility(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        return IsGfxLevelCore(level) ? api.QueryExtension("GL_ARB_ES3_compatibility") : false;
    }

    bool HasES31Compatibility(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        return IsGfxLevelCore(level) ? api.QueryExtension("GL_ARB_ES3_1_compatibility") : false;
    }

    bool HasDirectStateAccess(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore45))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_direct_state_access");
        return false;
    }

    bool HasComputeShader(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore43) || IsGfxLevelES(level, kGfxLevelES31))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_compute_shader") && api.QueryExtension("GL_ARB_shader_image_load_store") && api.QueryExtension("GL_ARB_shader_storage_buffer_object");
        return false;
    }

    bool HasVertexArrayObject(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        // We avoid as much as possible to use VAO on mobile because Qualcomm ES2 and ES3 drivers have random crashes on them
        // No extension checking and only use in ES 3.1 and core profile where it's required
        // Extension checking is required to run ES on Core context.

        if (IsGfxLevelCore(level) || IsGfxLevelES(level, kGfxLevelES31))
            return true;
        if (UNITY_DESKTOP)
            return api.QueryExtension("GL_ARB_vertex_array_object") || api.QueryExtension("GL_OES_vertex_array_object");
        return false;
    }

    bool HasIndirectParameter(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_ARB_indirect_parameters");
        return false;
    }

    bool HasBufferQuery(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore44))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_query_buffer_object") || api.QueryExtension("GL_AMD_query_buffer_object");
        return false;
    }

    bool HasIndirectDraw(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore40) || IsGfxLevelES(level, kGfxLevelES31))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_draw_indirect");
        return false;
    }

    bool HasMultiDrawIndirect(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore43))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_multi_draw_indirect") || api.QueryExtension("GL_ARB_multi_draw_indirect");
        return false;
    }

    bool HasInstancedDraw(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_NV_draw_instanced") || api.QueryExtension("GL_EXT_draw_instanced") || api.QueryExtension("GL_ARB_draw_instanced") || api.QueryExtension("ANGLE_instanced_arrays");
        return false;
    }

    bool HasDrawBaseVertex(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_draw_elements_base_vertex") || api.QueryExtension("GL_OES_draw_elements_base_vertex") || api.QueryExtension("GL_ARB_draw_elements_base_vertex");
        return false;
    }

    bool HasSeparateShaderObject(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore41) || IsGfxLevelES(level, kGfxLevelES31))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_separate_shader_objects") || api.QueryExtension("GL_EXT_separate_shader_objects");
        return false;
    }

    bool HasStencilTexturing(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore43) || IsGfxLevelES(level, kGfxLevelES31))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_stencil_texturing");
        return false;
    }

    bool HasSamplerObject(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore33) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_sampler_objects");
        return false;
    }

    bool HasAnisoFilter(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_EXT_texture_filter_anisotropic") || api.QueryExtension("EXT_texture_filter_anisotropic") || api.QueryExtension("WEBKIT_EXT_texture_filter_anisotropic");
        return false;
    }

    bool HasImageCopy(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore43) || IsGfxLevelES(level, kGfxLevelES31AEP))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_copy_image") || api.QueryExtension("GL_OES_copy_image") || api.QueryExtension("GL_EXT_copy_image");
        return false;
    }

    bool HasTextureMultisample(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES31))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_texture_multisample");
        return false;
    }

    bool HasBufferStorage(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore44))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_buffer_storage") || api.QueryExtension("GL_EXT_buffer_storage");
        return false;
    }

    bool HasUniformBuffer(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_uniform_buffer_object");
        return false;
    }

    bool HasProgramPointSize(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        return IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3);
    }

    int GetTransformFeedbackBufferBindings(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelES2(level))
            return 0;

        bool hasQueryAPI = false;
        if (IsGfxLevelCore(level, kGfxLevelCore40))
            hasQueryAPI = true;
        else if (!clamped)
            hasQueryAPI = api.QueryExtension("GL_ARB_transform_feedback3");

        int maxTransformFeedbackBufferBindings = 0;
        if (hasQueryAPI)
            maxTransformFeedbackBufferBindings = std::min<int>(api.Get(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS), gl::kMaxTransformFeedbackBufferBindings);

        if (IsGfxLevelCore(level, kGfxLevelCore40) && hasQueryAPI)
            return maxTransformFeedbackBufferBindings;
        if (IsGfxLevelCore(level, kGfxLevelCore32))
            return 1;
        if (IsGfxLevelES(level, kGfxLevelES3))
            return 1;
        return 0;
    }

    int GetMaxVertexUniforms(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelES2(level))
            return api.Get(GL_MAX_VERTEX_UNIFORM_VECTORS) * 4; // GLES2 expresses uniform variables with vector of 4 components
        else
            return api.Get(GL_MAX_VERTEX_UNIFORM_COMPONENTS);
    }

    bool HasBlitFramebuffer(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return (api.QueryExtension("GL_NV_framebuffer_blit") && api.QueryExtension("GL_NV_read_buffer")) || api.QueryExtension("GL_ARB_framebuffer_blit");
        return false;
    }

    bool HasFramebufferColorRead(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        // OSX drivers bug: OSX supports ES2_compability but generates an invalid operation error when calling glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT)
#		if UNITY_OSX
        return false;
#		endif

        if (IsGfxLevelES(level))
            return true;
        if (IsGfxLevelCore(level, kGfxLevelCore41))
            return !caps.gles.isIntelGpu; // Intel drivers (May 2015) have a bug and generates an invalid operation error on glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, ...)
        if (!clamped)
            return api.QueryExtension("GL_ARB_ES2_compatibility") && !caps.gles.isIntelGpu;
        return false;
    }

    bool HasMultisampleBlitScaled(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_EXT_framebuffer_multisample_blit_scaled");
        return false;
    }

    bool HasPackedDepthStencil(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (IsGfxLevelES2(level))
            return api.QueryExtension("GL_OES_packed_depth_stencil") || api.QueryExtension("GL_EXT_packed_depth_stencil");
        return false;
    }

    bool HasRenderTargetStencil(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (IsGfxLevelES2(level))
        {
#			if UNITY_BB10
            return false;
#			else
            // Adreno 2xx seems to not like a texture attached to color & depth & stencil at once;
            // Adreno 3xx seems to be fine. Most 3xx devices have GL_OES_depth_texture_cube_map extension
            // present, and 2xx do not. So detect based on that.
            const bool isAdreno2xx = caps.gles.isAdrenoGpu && !api.QueryExtension("GL_OES_depth_texture_cube_map");
            if (isAdreno2xx)
                return false;
#			endif

            return true;
        }

        return false;
    }

    bool HasInvalidateFramebuffer(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore43) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_invalidate_subdata") || api.QueryExtension("GL_EXT_discard_framebuffer");
        return false;
    }

    bool HasMipMaxLevel(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        return IsGfxLevelES2(level) ? api.QueryExtension("GL_APPLE_texture_max_level") : true;
    }

    bool HasMultiSampleAutoResolve(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_EXT_multisampled_render_to_texture") || api.QueryExtension("GL_IMG_multisampled_render_to_texture");
        return false;
    }

    bool HasMultisample(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
#		if UNITY_WIN
        // June 2015 - Intel Sandy Bridge generate an invalid operation error on glEnable(GL_MULTISAMPLE)
        const int version = caps.gles.majorVersion * 10 + caps.gles.minorVersion;
        if (caps.gles.isIntelGpu && version < 32)
            return false;
#		endif//UNITY_WIN

#		if UNITY_WEBGL
        // This should be in WebGL 2.0, but currently, in Firefox (the only WebGL 2.0 implementation), we get:
        // Error: WebGL: renderbufferStorageMultisample: Multisampling is still under development, and is currently disabled.
        // So, enable this when it is fixed.
        return false;
#		endif

        if (IsGfxLevelCore(level) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (HasMultiSampleAutoResolve(api, caps, level, clamped))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_framebuffer_object") || api.QueryExtension("GL_APPLE_framebuffer_multisample") || (api.QueryExtension("GL_NV_framebuffer_multisample") && api.QueryExtension("GL_NV_framebuffer_blit"));
        return false;
    }

    bool HasTextureSRGB(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_sRGB") || api.QueryExtension("GL_EXT_texture_sRGB");
        return false;
    }

    bool HasFramebufferSRGBEnable(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        // Intel June 2015 drivers don't expose GL_EXT_sRGB_write_control on their OpenGL ES 3.1 drivers but if we don't use glDisable(GL_FRAMEBUFFER_SRGB),
        // the drivers will performance a linear to sRGB conversion on the default framebuffer.

        if (UNITY_WIN && caps.gles.isIntelGpu)
            return true;

#if UNITY_ANDROID
        if (caps.gles.isIntelGpu && android::systeminfo::ApiLevel() < android::apiLollipop)
            return false; // generates GL errors on Baytrail when calling glDisable(GL_FRAMEBUFFER_SRGB).
#endif

        if (IsGfxLevelCore(level))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_sRGB_write_control") || api.QueryExtension("GL_ARB_framebuffer_sRGB");
        return false;
    }

    bool HasFramebufferSRGB(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        // June 2015: It's completely buggy all over the place, all vendors included and the Khronos Group is just clueless about sRGB.
        // For more information on this topic http://www.g-truc.net/post-0720.html#menu when we decide to fix whatever we color linear / gamma rendering.

#		if !(UNITY_WIN || UNITY_OSX || UNITY_LINUX)
        return false; // not implemented and/or not tested in other players
#		endif

        if (IsGfxLevelCore(level) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_sRGB") || api.QueryExtension("GL_ARB_framebuffer_sRGB");
        return false;
    }

    bool HasClearBufferFloat(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore41) || IsGfxLevelES(level))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_ES2_compatibility");
        return false;
    }

    bool HasTexSRGBDecode(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_EXT_texture_sRGB_decode");
        return false;
    }

    bool HasMapbuffer(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
#		if UNITY_WEBGL
        return false;
#		endif

        if (IsGfxLevelCore(level))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_OES_mapbuffer") || api.QueryExtension("GL_ARB_vertex_buffer_object");
        return false;
    }

    bool HasMapbufferRange(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
#		if UNITY_WEBGL
        return false;
#		endif

        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_map_buffer_range") || api.QueryExtension("GL_ARB_map_buffer_range");
        return false;
    }

    bool HasCircularBuffer(const ApiGLES & api, const GraphicsCapsGLES & capsGles, GfxDeviceLevelGL level, bool clamped)
    {
#if UNITY_ANDROID || UNITY_IOS || UNITY_TVOS
        // Android: Circular buffer is significantly slower in performance tests (DynamicBatchingLitCubes) on Adreno and Mali drivers
        // On Tegra K1 using circular buffers causes broken geometry
        // iOS/tvOS: Circular buffer seems to cause some gliches (case 785036) and in general not properly tested for perf
        return false;
#endif
        return HasMapbufferRange(api, level, clamped);
    }

    bool HasBufferCopy(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_copy_buffer");
        return false;
    }

    bool HasBufferClear(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore44))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_clear_buffer_object");
        return false;
    }

    bool Has16BitFloatVertex(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_OES_vertex_half_float");
        return false;
    }

    bool HasBlendAdvanced(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped || level == kGfxLevelES31AEP)
            return api.QueryExtension("GL_KHR_blend_equation_advanced") || api.QueryExtension("GL_NV_blend_equation_advanced");
        return false;
    }

    bool HasBlendAdvancedCoherent(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_KHR_blend_equation_advanced_coherent") || api.QueryExtension("GL_NV_blend_equation_advanced_coherent");
        return false;
    }

    bool HasDisjointTimerQuery(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        // Temp workaround for nvidia drivers throwing GL_INVALID_ENUM on this.
        // Bug confirmed by nvidia, they use a wrong enum value for this (0x8EBB instead of 0x8FBB)
        if (GetGraphicsCaps().gles.isNvidiaGpu || GetGraphicsCaps().gles.isTegraGpu)
            return false;

        // Disable timer queries on Adrenos until we get the drivers to actually work
        if (GetGraphicsCaps().gles.isAdrenoGpu)
            return false;

        if (!clamped)
            return api.QueryExtension("GL_EXT_disjoint_timer_query");
        return false;
    }

    bool HasTimerQuery(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        // Disable timer query on the editor for now, multiple contexts make it all confused
#		if UNITY_EDITOR
        return false;
#		endif

        if (IsGfxLevelCore(level, kGfxLevelCore33))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_timer_query") || api.QueryExtension("GL_NV_timer_query") || HasDisjointTimerQuery(api, level, clamped);
        return false;
    }

    bool HasInternalformat(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore43))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_internalformat_query2");
        return false;
    }

    bool HasS3TC(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_EXT_texture_compression_s3tc") || api.QueryExtension("WEBGL_compressed_texture_s3tc") || api.QueryExtension("WEBKIT_WEBGL_compressed_texture_s3tc");
        return false;
    }

    bool HasDXT1(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return HasS3TC(api, level, clamped) || api.QueryExtension("GL_EXT_texture_compression_dxt1") || api.QueryExtension("GL_ANGLE_texture_compression_dxt1");
        return false;
    }

    bool HasDXT3(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return HasS3TC(api, level, clamped) || api.QueryExtension("GL_CHROMIUM_texture_compression_dxt3") || api.QueryExtension("GL_ANGLE_texture_compression_dxt3");
        return false;
    }

    bool HasDXT5(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return HasS3TC(api, level, clamped) || api.QueryExtension("GL_CHROMIUM_texture_compression_dxt5") || api.QueryExtension("GL_ANGLE_texture_compression_dxt5");
        return false;
    }

    bool HasPVRTC(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_IMG_texture_compression_pvrtc") || api.QueryExtension("WEBGL_compressed_texture_pvrtc");
        return false;
    }

    bool HasATC(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_AMD_compressed_ATC_texture") || api.QueryExtension("GL_ATI_texture_compression_atitc") || api.QueryExtension("WEBGL_compressed_texture_atc");
        return false;
    }

    bool HasASTC(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped || level == kGfxLevelES31AEP)
            return api.QueryExtension("GL_KHR_texture_compression_astc_ldr") || api.QueryExtension("WEBGL_compressed_texture_astc_ldr");
        return false;
    }

    bool HasETC1(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        if (!clamped)
            return api.QueryExtension("GL_OES_compressed_ETC1_RGB8_texture") || api.QueryExtension("WEBGL_compressed_texture_etc1");
        return false;
    }

    bool HasETC2AndEAC(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        // ETC2 and EAC are in OpenGL 4.3 through GL_ARB_ES3_compatibility but most desktop implementations support these formats through online decompression
        // In March 2015, AMD had no intention to support any format of ETC2 or EAC in their drivers
        if (caps.gles.isAMDGpu)
            return false;
        if (IsGfxLevelCore(level, kGfxLevelCore43) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_ARB_ES3_compatibility") || api.QueryExtension("WEBGL_compressed_texture_es3");
        return false;
    }

    bool HasFloatTexture(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_OES_texture_float") || api.QueryExtension("GL_ARB_texture_float");
        return false;
    }

    bool HasHalfTexture(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_OES_texture_half_float") || api.QueryExtension("GL_ARB_texture_float");
        return false;
    }

    bool HasTextureRG(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_texture_rg") || api.QueryExtension("GL_ARB_texture_rg");
        return false;
    }

    bool HasRenderToFloatTexture(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_color_buffer_float") || api.QueryExtension("GL_ARB_texture_float") || api.QueryExtension("WEBGL_color_buffer_float");
        return false;
    }

    bool HasRenderToHalfTexture(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
            return api.QueryExtension("GL_EXT_color_buffer_half_float") || api.QueryExtension("GL_ARB_texture_float");
        return false;
    }

    bool HasRenderToRG11B10Texture(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level, bool clamped)
    {
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;
        if (!clamped)
        {
            // apple demands both extensions to be present
            if (api.QueryExtension("GL_APPLE_color_buffer_packed_float") && api.QueryExtension("GL_APPLE_texture_packed_float"))
                return true;

            return api.QueryExtension("GL_EXT_packed_float");
        }
        return false;
    }

    bool HasNativeDepthTexture(const ApiGLES & api, const GraphicsCaps & caps, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelES2(level))
        {
#			if UNITY_ANDROID
            if (android::systeminfo::ApiLevel() < android::apiHoneycomb || (android::systeminfo::ApiLevel() < android::apiIceCreamSandwich && caps.gles.isPvrGpu))
                return false;
#			endif
#			if UNITY_ANDROID && defined(__i386__)
            if (caps.gles.isPvrGpu && android::systeminfo::ApiLevel() < android::apiLollipop) // avoid buggy drivers on Intel devices
                return false;
#			endif

            return api.QueryExtension("GL_OES_depth_texture") || api.QueryExtension("GL_GOOGLE_depth_texture") || api.QueryExtension("WEBGL_depth_texture") || api.QueryExtension("GL_WEBGL_depth_texture") || api.QueryExtension("GL_ARB_depth_texture");;
        }
        return true;
    }

    bool HasTexLodSamplers(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelES2(level))
            return api.QueryExtension("GL_EXT_shader_texture_lod");

        return true;
    }

    bool HasStereoscopic3D(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelCore(level))
            return api.Get(GL_STEREO) == GL_TRUE;
        return false;
    }

    bool HasFenceSync(const ApiGLES &api, GfxDeviceLevelGL level)
    {
        // NOTE: fences are quite slow (even just for checking whether we've passed them) on OSX NVidias.
        if (IsGfxLevelCore(level, kGfxLevelCore32) || IsGfxLevelES(level, kGfxLevelES3))
            return true;

        return false;
    }

    bool HasHighpFloatInFragmentShader(ApiGLES *api, GfxDeviceLevelGL level)
    {
        if (IsGfxLevelES2(level))
        {
            GLint range[2];
            GLint precision;
            GLES_CALL(api, glGetShaderPrecisionFormat, GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
            //gles2 will return zero for both range and precision if high float is not supported
            return !(precision == 0 && range[0] == 0 && range[1] == 0);
        }

        return true;
    }

    int GetMaxAnisoSamples(const ApiGLES & api, GfxDeviceLevelGL level, bool clamped)
    {
        bool hasAPI = !clamped && (api.QueryExtension("GL_EXT_texture_filter_anisotropic") || api.QueryExtension("WEBKIT_EXT_texture_filter_anisotropic"));

        return hasAPI ? api.Get(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT) : 1;
    }

    int GetMaxColorAttachments(const ApiGLES & api, GfxDeviceLevelGL level)
    {
        if (!HasDrawBuffers(api, level))
            return 1;

        return clamp<int>(api.Get(GL_MAX_COLOR_ATTACHMENTS), 1, kMaxSupportedRenderTargets);
    }

 
    void InitVersion(const ApiGLES& api, GfxDeviceLevelGL level, int& majorVersion, int& minorVersion)
    {
        majorVersion = 0;
        minorVersion = 0;

        if (IsGfxLevelES2(level))
        {
            majorVersion = 2;
            minorVersion = 0;
        }
        else
        {
            majorVersion = api.Get(GL_MAJOR_VERSION);
            minorVersion = api.Get(GL_MINOR_VERSION);
        }
    }

    // Tweaks GL level downwards if not supported by current context
    GfxDeviceLevelGL AdjustAPILevel(GfxDeviceLevelGL level, int majorVersion, int minorVersion)
    {
        GfxDeviceLevelGL maxSupported = kGfxLevelCoreLast;
        if (level < kGfxLevelCoreFirst)
        {
            if (majorVersion == 2)
                maxSupported = kGfxLevelES2;
            if (majorVersion == 3 && minorVersion == 0)
                maxSupported = kGfxLevelES3;
        }
        else
        {
            if (majorVersion == 3)
            {
                maxSupported = kGfxLevelCore32;
                if (minorVersion >= 3)
                    maxSupported = kGfxLevelCore33;
            }
            else if (majorVersion == 4)
            {
                maxSupported = (GfxDeviceLevelGL)(kGfxLevelCore40 + (GfxDeviceLevelGL)minorVersion);
            }
        }
        return std::min(maxSupported, level);
    }

    bool IsOpenGLES2OnlyGPU(const GfxDeviceLevelGL level, const std::string& renderer)
    {
        if (!IsGfxLevelES2(level))
            return false;

        const char* const es2GpuStrings[] =
        {
            "Mali-200",
            "Mali-300",
            "Mali-400",
            "Mali-450",
            "PowerVR SGX",
            "Adreno (TM) 2",
            "Tegra 3",
            "Tegra 4",
            "Vivante GC1000",
            "VideoCore IV",
            "Bluestacks"
        };

        for (int i = 0; i < ARRAY_SIZE(es2GpuStrings); ++i)
        {
            if (renderer.find(es2GpuStrings[i]) != std::string::npos)
                return true;
        }
        return false;
    }

}//namespace

GraphicsCapsGLES::GraphicsCapsGLES()
{
    memset(this, 0x00000000, sizeof(GraphicsCapsGLES));
    // cannot do any non-zero initialization here, GraphicCaps::GraphicsCaps() will memset it to 0 later.
}

namespace gles
{
    void InitRenderTextureFormatSupport(ApiGLES * api, GraphicsCaps* caps)
    {
        const bool hasRG = ::HasTextureRG(*api, *caps, caps->gles.featureLevel, caps->gles.featureClamped);
        const bool hasRTFloat = ::HasRenderToFloatTexture(*api, *caps, caps->gles.featureLevel, caps->gles.featureClamped);
        const bool hasRTHalf = ::HasRenderToHalfTexture(*api, *caps, caps->gles.featureLevel, caps->gles.featureClamped);
        const bool hasRTRG11B10 = ::HasRenderToRG11B10Texture(*api, *caps, caps->gles.featureLevel, caps->gles.featureClamped);
        const bool hasTexRGB_A2 = IsGfxLevelES2(caps->gles.featureLevel) ? api->QueryExtension("GL_EXT_texture_type_2_10_10_10_REV") : true;
        const bool hasTexInt = !IsGfxLevelES2(caps->gles.featureLevel);
    }

    void InitCaps(ApiGLES * api, GraphicsCaps* caps, GfxDeviceLevelGL &level)
    {
        g_GraphicsCapsGLES = &caps->gles;
        ::InitVersion(*api, level, caps->gles.majorVersion, caps->gles.minorVersion);

        GetGraphicsCaps().gles.featureLevel = level = ::AdjustAPILevel(level, caps->gles.majorVersion, caps->gles.minorVersion);

        // we want white for missing color channel instead of gl default black
        // Also we want normals and tangents not to be all zeroes.
        caps->requiredShaderChannels = VERTEX_FORMAT3(Normal, Color, Tangent);

        //caps->vendorString = api->GetDriverString(gl::kDriverQueryVendor);
        //caps->rendererString = api->GetDriverString(gl::kDriverQueryRenderer);
        //caps->driverVersionString = api->GetDriverString(gl::kDriverQueryVersion);

        caps->gles.featureClamped = false;// HasARGV("force-clamped");
        const bool clamped = caps->gles.featureClamped;

        caps->gles.hasWireframe = IsGfxLevelCore(level);

        caps->usesOpenGLTextureCoords = true;
        caps->gles.isPvrGpu = (caps->rendererString.find("PowerVR") != std::string::npos);
        caps->gles.isMaliGpu = (caps->rendererString.find("Mali") != std::string::npos);
        caps->gles.isAdrenoGpu = (caps->rendererString.find("Adreno") != std::string::npos);
        caps->gles.isTegraGpu = (caps->rendererString.find("Tegra") != std::string::npos);
        caps->gles.isIntelGpu = (caps->rendererString.find("Intel") != std::string::npos);
        caps->gles.isNvidiaGpu = (caps->rendererString.find("NVIDIA") != std::string::npos);
        caps->gles.isAMDGpu = (caps->rendererString.find("AMD") != std::string::npos) || (caps->rendererString.find("ATI") != std::string::npos);
        caps->gles.isVivanteGpu = (caps->rendererString.find("Vivante") != std::string::npos);
        caps->gles.isES2Gpu = IsOpenGLES2OnlyGPU(level, caps->rendererString);

        caps->hasTiledGPU = caps->gles.isPvrGpu || caps->gles.isAdrenoGpu || caps->gles.isMaliGpu || caps->gles.isVivanteGpu;

#		if UNITY_ANDROID
        caps->gles.haspolygonOffsetBug = (caps->gles.driverGLESVersion == 2 && (caps->gles.isMaliGpu || caps->gles.isPvrGpu));
        //#			if DEBUGMMODE
        //				if(caps->gles.haspolygonOffsetBug)
        //					printf_console("Activating PolygonOffset compensation\n");
        //#			endif
#		endif // if UNITY_ANDROID

#		if UNITY_APPLE_PVR
            // Apple started to brand GPUs as Apple GPU on newer devices. For now let's pretend this is pvr (and it is actually)
        caps->gles.isPvrGpu = true;
        caps->hasTiledGPU = true;
        caps->hasHiddenSurfaceRemovalGPU = true;
#		endif // if UNITY_IPHONE

        caps->gles.useDiscardToAvoidRestore = GetGraphicsCaps().gles.isAdrenoGpu;
        caps->gles.useClearToAvoidRestore = GetGraphicsCaps().gles.isPvrGpu || GetGraphicsCaps().gles.isMaliGpu;

        caps->gles.hasNVNLZ = !clamped && api->QueryExtension("GL_NV_depth_nonlinear");
        caps->gles.hasNVCSAA = !clamped && api->QueryExtension("GL_NV_coverage_sample");

        caps->gles.hasIndirectDraw = ::HasIndirectDraw(*api, level, clamped);
        caps->gles.hasDrawBaseVertex = ::HasDrawBaseVertex(*api, level, clamped);
        caps->gles.hasMultiDrawIndirect = ::HasMultiDrawIndirect(*api, level, clamped);


        caps->gles.hasDirectStateAccess = ::HasDirectStateAccess(*api, level, clamped);
        caps->gles.buggyDSASampleQueryAMD = ((caps->gles.majorVersion * 10 + caps->gles.minorVersion) >= 45) && caps->gles.isAMDGpu;

        // -- Handle tokens referencing the same feature between ES2 and other GL API --
        caps->gles.memoryBufferTargetConst = ::HasBufferCopy(*api, level, clamped) ? gl::kCopyWriteBuffer : gl::kArrayBuffer;

        // -- sRGB features --

        caps->gles.hasTextureSrgb = ::HasTextureSRGB(*api, level, clamped);
        caps->gles.hasTexSRGBDecode = ::HasTexSRGBDecode(*api, level, clamped);
        caps->gles.hasDxtSrgb = (IsGfxLevelCore(level) && HasS3TC(*api, level, clamped)) || api->QueryExtension("GL_NV_sRGB_formats");
        caps->gles.hasPvrSrgb = IsGfxLevelES(level) && api->QueryExtension("GL_EXT_pvrtc_sRGB");

        caps->gles.hasInvalidateFramebuffer = ::HasInvalidateFramebuffer(*api, level, clamped);
        caps->gles.hasBlitFramebuffer = ::HasBlitFramebuffer(*api, level, clamped);
        caps->gles.hasReadDrawFramebuffer = caps->gles.hasBlitFramebuffer || api->QueryExtension("GL_APPLE_framebuffer_multisample");
        caps->gles.hasMultisampleBlitScaled = ::HasMultisampleBlitScaled(*api, level, clamped);
        caps->gles.requireDrawBufferNone = ::RequireDrawBufferNone(*api, *caps, level);
        caps->gles.hasDrawBuffers = ::HasDrawBuffers(*api, level);
        caps->gles.hasDepth24 = IsGfxLevelES2(level) ? api->QueryExtension("GL_OES_depth24") : true;
        caps->gles.hasFramebufferColorRead = ::HasFramebufferColorRead(*api, *caps, level, clamped);
        caps->gles.hasFramebufferSRGBEnable = ::HasFramebufferSRGBEnable(*api, *caps, level, clamped);

        // ES 3 spec says that "FRAMEBUFFER is equivalent to DRAW_FRAMEBUFFER" when passed as target to glFramebufferTexture2D etc., but the Vivante ES3 driver generates errors
        caps->gles.framebufferTargetForBindingAttachments = caps->gles.hasReadDrawFramebuffer && caps->gles.isVivanteGpu ? GL_DRAW_FRAMEBUFFER : GL_FRAMEBUFFER;
        caps->gles.requireClearAlpha = false;

        caps->gles.hasClearBufferFloat = ::HasClearBufferFloat(*api, *caps, level, clamped);
#
        // -- Buffer features --

        caps->gles.hasMapbuffer = ::HasMapbuffer(*api, level, clamped);
        caps->gles.hasMapbufferRange = ::HasMapbufferRange(*api, level, clamped);
        caps->gles.hasBufferCopy = ::HasBufferCopy(*api, level, clamped);
        caps->gles.hasVertexArrayObject = ::HasVertexArrayObject(*api, level, clamped);
        caps->gles.hasIndirectParameter = ::HasIndirectParameter(*api, level, clamped);
        caps->gles.hasBufferQuery = ::HasBufferQuery(*api, level, clamped);
        caps->gles.hasBufferClear = ::HasBufferClear(*api, level, clamped);
        caps->gles.hasCircularBuffer = ::HasCircularBuffer(*api, caps->gles, level, clamped);


        caps->gles.hasES2Compatibility = HasES2Compatibility(*api, level);
        caps->gles.hasES3Compatibility = HasES3Compatibility(*api, level);
        caps->gles.hasES31Compatibility = HasES31Compatibility(*api, level);
        caps->gles.useHighpDefaultFSPrec = HasHighpFloatInFragmentShader(api, level);
        caps->gles.hasTexLodSamplers = HasTexLodSamplers(*api, level);

        caps->gles.hasFenceSync = HasFenceSync(*api, level);

#		if UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN
        // Disable uniform buffers on Adreno 3xx's. ES 3.1 GPUs should be fine.
        caps->gles.buggyUniformBuffers = caps->gles.isAdrenoGpu && caps->driverVersionString.find("OpenGL ES 3.0") != std::string::npos;
#		else
        caps->gles.buggyUniformBuffers = false;
#		endif

#		if UNITY_ANDROID
        caps->gles.buggyGeomShadersInBinaryPrograms = caps->gles.isAdrenoGpu;
        caps->gles.buggyTexStorageNonSquareMipMapETC = IsGfxLevelES3(level) && caps->gles.isAdrenoGpu && android::systeminfo::ApiLevel() < 21;
#		else
        caps->gles.buggyGeomShadersInBinaryPrograms = false;
        caps->gles.buggyTexStorageNonSquareMipMapETC = false;
#		endif

        caps->gles.buggyTexStorageDXT = caps->gles.isVivanteGpu;

#		if UNITY_ANDROID
        caps->gles.buggyBindElementArrayBuffer = caps->gles.isVivanteGpu && android::systeminfo::ApiLevel() < 19; // bug was never seen on Android 4.4 (API 19)
#		endif

#		if UNITY_IOS || UNITY_TVOS
            // both gles2 and gles3 have issue with non-full mipchains (up to some ios9.x)
            // though we can do workaround ONLY if there is texture_max_level
        caps->gles.buggyTextureStorageWithNonFullMipChain = HasMipMaxLevel(*api, level);
#		endif

        // Fill the shader language table
        caps->gles.supportedShaderTagsCount = 0;
        if (IsGfxLevelCore(GetGraphicsCaps().gles.featureLevel))
        {
            caps->gles.supportedShaderTags[caps->gles.supportedShaderTagsCount++] = "glcore ";
#			if 0
            // To run OpenGL ES shaders on OpenGL core context which is not supported yet and cause errors on OSX
            // because we generate OpenGL ES shaders that relies EXT_shadow_samplers which isn't support on desktop in the extension form.
            // Also supporting gles or gles3 shaders over glcore will make the wrong shader be loaded by tags - depending on which is
            // encountered first during loading. If glcore shaders are not functionaly equivalent to gles shaders, this will cause bugs.
            if (api->QueryExtension("GL_ARB_ES2_compatibility"))
                caps->gles.supportedShaderTags[caps->gles.supportedShaderTagsCount++] = "gles ";
            if (api->QueryExtension("GL_ARB_ES3_compatibility"))
                caps->gles.supportedShaderTags[caps->gles.supportedShaderTagsCount++] = "gles3 ";
#			endif
        }
        else if (IsGfxLevelES2(GetGraphicsCaps().gles.featureLevel))
        {
            caps->gles.supportedShaderTags[caps->gles.supportedShaderTagsCount++] = "gles ";
        }
        else if (IsGfxLevelES(GetGraphicsCaps().gles.featureLevel)) // ES3 and up
        {
            caps->gles.supportedShaderTags[caps->gles.supportedShaderTagsCount++] = "gles3 ";
        }

        caps->gles.maxVertexUniforms = ::GetMaxVertexUniforms(*api, level);

        caps->gles.hasBufferStorage = ::HasBufferStorage(*api, level, clamped);
        caps->gles.hasUniformBuffer = (!caps->gles.buggyUniformBuffers) && ::HasUniformBuffer(*api, level, clamped);
        if (caps->gles.hasUniformBuffer)
        {
            caps->gles.maxUniformBlockSize = api->Get(GL_MAX_UNIFORM_BLOCK_SIZE);
            caps->gles.maxUniformBufferBindings = std::min<int>(api->Get(GL_MAX_UNIFORM_BUFFER_BINDINGS), gl::kMaxUniformBufferBindings);
        }

        // Our instancing implementation requires working uniform buffers
        caps->hasInstancing = ::HasInstancedDraw(*api, level, clamped) && caps->gles.hasUniformBuffer;

        // Instanced draw doesn't render on older macs...clamp to gl4.1
        if (IsGfxLevelCore(level))
            caps->hasInstancing &= IsGfxLevelCore(level, kGfxLevelCore41);

        caps->gles.useActualBufferTargetForUploads = UNITY_WEBGL || caps->gles.isMaliGpu;

        caps->gles.maxAttributes = std::min<int>(api->Get(GL_MAX_VERTEX_ATTRIBS), gl::kVertexAttrCount);

#		if UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN
        // Adreno (Qualcomm chipsets) have a driver bug (Android2.2)
        // which fails to patch vfetch instructions under certain circumstances
        caps->gles.buggyVFetchPatching = caps->gles.isAdrenoGpu;
#		endif
    }
}//namespace gles
