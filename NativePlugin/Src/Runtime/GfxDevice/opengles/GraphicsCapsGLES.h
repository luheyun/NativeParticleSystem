#pragma once

#include "Runtime/Utilities/fixed_array.h"
#include "Runtime/Math/Rect.h"
#include "ApiEnumGLES.h"

enum GraphicsProfileGL
{
	kProfileDefault,
	kProfileCompatibility,
	kProfileCore,
	kProfileES
};

enum
{
	kMaxShaderTags = 10
};

// Returns true if the param is one of GL ES device levels.
inline bool IsGfxLevelES2(GfxDeviceLevelGL level, GfxDeviceLevelGL minLevel = kGfxLevelES2First)
{
	return level >= minLevel && level <= kGfxLevelES2Last;
}

inline bool IsGfxLevelES3(GfxDeviceLevelGL level, GfxDeviceLevelGL minLevel = kGfxLevelES3First)
{
	return level >= minLevel && level <= kGfxLevelES3Last;
}

inline bool IsGfxLevelES(GfxDeviceLevelGL level, GfxDeviceLevelGL minLevel = kGfxLevelESFirst)
{
	return level >= minLevel && level <= kGfxLevelESLast;
}

inline bool IsGfxLevelCore(GfxDeviceLevelGL level, GfxDeviceLevelGL minLevel = kGfxLevelCoreFirst)
{
	return level >= minLevel && level <= kGfxLevelCoreLast;
}

struct GraphicsCapsGLES
{
	GraphicsCapsGLES();

	GfxDeviceLevelGL featureLevel;
	bool featureClamped;

	// caps

	int		maxAttributes;
	int		maxAASamples;
	int		hasVertexShaderTexUnits;
	int		maxUniformBufferBindings;
	int		maxTransformFeedbackBufferBindings;
	int		maxShaderStorageBufferBindings;
	int		maxAtomicCounterBufferBindings;

	int		maxUniformBlockSize;
	int		maxVertexUniforms;

	int		driverGLESVersion;

	fixed_array<const char *, kMaxShaderTags> supportedShaderTags;
	int		supportedShaderTagsCount;

	// handling of different constants in gles2/gles3

	gl::BufferTarget memoryBufferTargetConst;	// If copy buffer isn't supported, we use a different constant

	// gpu

	bool	isPvrGpu;
	bool	isMaliGpu;
	bool	isAdrenoGpu;
	bool	isTegraGpu;
	bool	isIntelGpu;
	bool	isNvidiaGpu;
	bool	isAMDGpu;
	bool	isVivanteGpu;
	bool	isES2Gpu;		// A GPU that supports only OpenGL ES 2.0

	bool	useDiscardToAvoidRestore;	// if discard/invalidate are preferred to avoid restore (usually means clear is fullscreen quad)
	bool	useClearToAvoidRestore;		// if clear is preferred to avoid restore
	bool	haspolygonOffsetBug;		// Need to increase polygon offset by a large factor

	// seems easier and more straightforward to have per-format srgb bools
	bool	hasTextureSrgb;
	
	// gl from GL_EXT_texture_sRGB_decode (Todo: could be done with texture views too from 4.3 or GL_ARB_texture_view
	bool	hasTexSRGBDecode; // Read sRGB textures directly without decoding: http://www.opengl.org/registry/specs/EXT/texture_sRGB_decode.txt
	bool	hasDxtSrgb;
	bool	hasPvrSrgb;

	// -- features/extensions --

	bool	hasES2Compatibility;			// To run GLSL ES 100 shader on desktop
	bool	hasES3Compatibility;			// To run GLSL ES 300 shader on desktop
	bool	hasES31Compatibility;			// To run GLSL ES 310 shader on desktop
	bool	hasBinaryShader;				// GL 4.1 / ES3 / GL_ARB_get_program_binary / GL_OES_get_program_binary
	bool	hasBinaryShaderRetrievableHint;	// GL 4.1 / ES3 / GL_ARB_get_program_binary
	bool	hasGeometryShader;				// GL 3.2 / GL_ARB_geometry_shader / GL_OES_geometry_shader / GL_EXT_geometry_shader
	bool	hasTessellationShader;			// GL 4.0 / GL_ARB_tessellation_shader / GL_OES_tessellation_shader / GL_EXT_tessellation_shader
	bool	hasUniformBuffer;				// GL 3.2 / GL_ARB_uniform_buffer_object / ES 3.0 / GL_IMG_uniform_buffer_object
	bool	hasBufferStorage;				// GL 4.4 / GL_ARB_buffer_storage / GL_EXT_texture_storage (ES)

	bool	hasDebugKHR;		// KHR_debug: Debug output + Debug Marker + Debug Label + Notification + Debug enable
	bool	hasDebugOutput;		// ARB_debug_output (desktop)
	bool	hasDebugMarker;		// EXT_debug_marker (ES)
	bool	hasDebugLabel;		// EXT_debug_label (ES)

	// glBlitFramebuffer support
	bool	hasBlitFramebuffer; // GL 3.0 / GL_ARB_framebuffer_object / GL_NV_framebuffer_blit / ES 3.0
	// GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER support (glBlitFramebuffer support or GL_APPLE_framebuffer_multisample)
	bool	hasReadDrawFramebuffer;
	// Should be passed as target to glFramebufferTexture2D, glFramebufferRenderbuffer etc. when binding the framebuffer using ApiGLES::BindFramebuffer and kDrawFramebuffer.
	unsigned int	framebufferTargetForBindingAttachments;

	bool	hasFramebufferSRGBEnable;		// GL_ARB_framebuffer_sRGB || GL_EXT_sRGB_write_control
	bool	hasMultisampleBlitScaled;		// EXT_framebuffer_multisample_blit_scaled (GL)
	bool	hasInvalidateFramebuffer;		// GL 4.2 / GL_ARB_invalidate_subdata  / ES 3.0 / GL_EXT_discard_framebuffer
	bool	hasDrawBuffers;					// GL 2.0 / ES3 / ES2 / GL_NV_draw_buffers / GL_EXT_draw_buffers

	// OpenGL desktop require to call glDrawBuffer(GL_NONE) on depth only framebuffer. This retriction was dropped in OpenGL 4.1 and ES2_compatibility
	bool	requireDrawBufferNone;

	bool	hasDepth24;							// GL 3.2 / ES 3.0 / GL_OES_depth24

	gl::TextureSwizzleMode hasTextureSwizzle;	// ES3 / GL3.3 / ARB_texture_swizzle / APPLE_texture_format_BGRA8888 / EXT_texture_format_BGRA8888
	bool	hasTextureStorage;					// GL 4.2 / ES 3.0 / GL_ARB_texture_storage / GL_EXT_texture_storage
	bool	hasTextureAlpha;					// Not in core profile. We need to used GL_R8 + texture swizzle
	bool	hasTextureMultisample;				// GL 3.2 / ES 3.1 / ARB_texture_multisample

	bool	hasMipBaseLevel;					// GL 3.2 / ES 3.0: GL_TEXTURE_BASE_LEVEL is supported
	bool	hasImageCopy;						// GL 4.3 / ES 3.2 / GL_ARB_copy_image / GL_OES_copy_image / GL_EXT_copy_image

	// ES 2.0 doesn't have seamless cubemap filtering and can't be enable
	// ES 3.0 has seamless cubemap filtering but it doesn't need to be enabled
	bool	hasSeamlessCubemapEnable;			// GL 3.2 / ARB_seamless_cube_map
	
	bool	hasAlphaLumTexStorage;				// ES 3.0 drops alpha/alpha_lum tex storage support, while ES 2.0 have it

	bool	hasMapbuffer;						// GL 1.5 / OES_mapbuffer / ES 3.0 droped it. We typically try not to use it.
	bool	hasMapbufferRange;					// GL 3.0 / ARB_map_buffer_range / ES 3.0 / EXT_map_buffer_range
	bool	hasBufferQuery;						// GL 4.4 / ARB_query_buffer_object / AMD_query_buffer_object
	bool	hasBufferCopy;						// GL 3.1 / ARB_copy_buffer /  ES 3.0
	bool	hasIndirectParameter;				// ARB_indirect_parameter
	bool	hasBufferClear;						// GL 4.4 / ARB_clear_buffer_object

	bool	hasCircularBuffer;					// Unity implementation of circular buffers. Depends on GL feature (mapBufferRange). Only enabled on drivers where it's actually fast

	bool	hasVertexArrayObject;				// GL 3.0 / ARB_vertex_array_object / ES 3.0 / OES_vertex_array_object
	bool	hasSeparateShaderObject;			// GL 4.1 / GL_ARB_separate_shader_objects / ES 3.1

	bool	hasIndirectDraw;					// GL 4.0 / ARB_draw_indirect / ES 3.1
	bool	hasMultiDrawIndirect;				// GL 4.3 / ARB_multi_draw_indirect / EXT_multi_draw_indirect (ES)
	bool	hasDrawBaseVertex;					// GL 3.2 / ARB_draw_elements_base_vertex / ES 3.2 / OES_draw_elements_base_vertex / EXT_draw_elements_base_vertex

	bool	hasPackedDepthStencil;				// GL 3.0 / GL_OES_packed_depth_stencil / GL_EXT_packed_depth_stencil / WebGL / ES 3.0
	bool	hasStencilTexturing;				// GL 4.3 / ARB_stencil_texturing / ES 3.1

	bool	hasFenceSync;						// GL 3.2, ES3.0 onwards / ARB_sync

	bool	hasProgramPointSize;				// GL 3.2, ES 3.0

	// workarounds and driver caveats

	// Driver BUG: AMD initial OpenGL 4.5 driver (4.5.13399) generates an invalid enum when calling glGetNamedFramebufferParameteriv with GL_SAMPLES
	bool	buggyDSASampleQueryAMD;

	// old gles2 adreno drivers fails to re-patch vfetch instructions
	// if shader stays the same, but vertex layout has been changed
	// will result in forced program change on draw call
	bool	buggyVFetchPatching;

	// if we should force highp as default precision
	// on Tegra there is a bug that sometimes results in samplers not being reported (workaround)
	// on Adreno (N4 at least) there is a bug that sometimes results in program link crash (workaround)
	bool	useHighpDefaultFSPrec;

	// True if the device has texture sampler with explicit LOD.
	bool	hasTexLodSamplers;

	// older imgtec android shader compiler have borken GL_EXT_shader_texture_lod
	// it insists on textureCubeLod instead of textureCubeLodEXT and textureCubeGradARB instead of textureCubeGradEXT
	// they told us it was fixed in reference driver and backported to driver ver 1.10@2570425
	// on the other hand we better off not trusting driver string, so we recheck the case on all pvr androids
	bool	buggyTexCubeLodGrad;

	// Some implementations may behave strangly if we don't reset BindBuffer(0) after edit a buffer.
	// Also this is not something we obversed but part of the code legacy.
	// The idea force the code to constently rebind buffer previously bound which increase CPU overhead.
	// We created buggyBindBuffer to keep the previous behaviour if necessary but avoid buffer rebinding
	// in general.
	//
	// This issue is know on iOS
	// iOS doesn't like to have the same buffer bound to the same binding point with both BindBuffer and BindBufferBase (below)
	bool	buggyBindBuffer;

	// on some adrenos (gles2) there is a bug in texture upload code, which usually hits font rendering
	// it seems there is some optimization inside a driver when you have several uploads one after another
	// and it is totally buggy
	// as a workaround we just call glFinish before the first texture upload of a frame
	bool	buggyTextureUploadSynchronization;

	// In some implementations glColorMask(0, 0, 0, 0) does not work in all situations.
	// E.g. on PowerVR SGX 540 when 'discard' is used in the shader.
	bool	buggyDisableColorWrite;

	// Adreno 3xx's are super slow when using uniform buffers. Also they crash (at least pre-lollipop) when trying to query
	// the bound uniform buffers with glGetIntegeri_v. If this is set, no checks will be made.
	bool	buggyUniformBuffers;

	// Adreno 4xx's (at least in Note 4) don't know how to return GL_GEOMETRY_LINKED_INPUT_TYPE if the program came from binary shader.
	// So disable binary shaders whenever a geometry shader is involved.
	bool	buggyGeomShadersInBinaryPrograms;

	// Adreno ES3 driver on Android 4.x gives error GL_INVALID_OPERATION when calling glCompressedTexSubImage2D
	// for low mip map levels of non-square ETC textures when using texture storage
	// (uploading only complete blocks as mentioned in the spec does not help).
	bool	buggyTexStorageNonSquareMipMapETC;

	// Vivante ES 3.0 driver does not support tex storage with DXT compressed textures.
	bool	buggyTexStorageDXT;

	// Vivante GC1000 driver sometimes changes the GL_ARRAY_BUFFER_BINDING when calling glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer)
	bool	buggyBindElementArrayBuffer;

	// iOS: some ios versions combo with some GPUs have issues with non-full mipchain coupled with GL_EXT_texture_storage
	// in that case we will specify *full* mipchain for glTexStorage, BUT still tweak GL_TEXTURE_MAX_LEVEL
	bool	buggyTextureStorageWithNonFullMipChain;

    //		If set, uses actual buffer target for data uploads instead of COPY_WRITE_BUFFER etc.
    bool	useActualBufferTargetForUploads;

	// BlackBerry adreno needs an extra call of this
	bool	requirePrepareFramebuffer;

	// The platform blend the framebuffer so we need to clear the Alpha channel that we don't typically keep clean.
	bool	requireClearAlpha;

    bool	hasWireframe; // GL

	bool	hasClearBufferFloat; // ES has glClearDepthf, GL < 4.1 has glClearDepth

	bool	hasDisjointTimerQuery; // GL_EXT_disjoint_timer_query

	// Requires ES2_compatibility on desktop. Also Intel drivers (May 2015) have a bug and generates an invalid operation error on glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, ...)
	bool	hasFramebufferColorRead;

	bool	hasInternalformat;		// GL 4.1: ARB_internalformat_query2

	bool	hasDirectStateAccess;	// OpenGL desktop, ARB: GL4.5

	// vendor-specific extensions
	// we separate them simply for clarity

	bool	hasNVNLZ;
	bool	hasNVMRT;				// gles2: special attachment points
	bool	hasNVCSAA;				// gles2: Tegra 3 has coverage sampling anti-aliasing

	int		majorVersion;			// Major OpenGL version, eg OpenGL 4.2 major version is 4
	int		minorVersion;			// Minor OpenGL version, eg OpenGL 4.2 minor version is 2
};

extern GraphicsCapsGLES* g_GraphicsCapsGLES;
