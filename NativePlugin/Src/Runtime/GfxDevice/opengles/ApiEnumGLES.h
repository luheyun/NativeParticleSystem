#pragma once

#include "Runtime/GfxDevice/GfxDeviceTypes.h"

// This define allows building the OpenGL back-end as if we were building WebGL. Debugging helper should be used with -force-gles20 and -force-clamped
#define FORCE_DEBUG_BUILD_WEBGL 0

namespace gl
{
	enum DriverQuery // Do not reorder, it would break the code!
	{
		kDriverQueryVendor,
		kDriverQueryRenderer,
		kDriverQueryVersion
	};

	enum SubmitMode
	{
		SUBMIT_FLUSH,
		SUBMIT_FINISH
	};

	enum EnabledCap
	{
		kBlend, kEnabledCapFirst = kBlend,
		kColorLogicOp,
		kCullFace,
		kDebugOutput,
		kDebugOutputSynchronous,
		kDepthClamp,
		kDepthTest,
		kDither,
		kFramebufferSRGB,
		kLineSmooth,
		kMultisample,
		kPolygonOffsetFill,
		kPolygonOffsetLine,
		kPolygonOffsetpoint,
		kPolygonSmooth,
		kPrimitiveRestart,
		kPrimitiveRestartFixedIndex,
		kRasterizerDiscard,
		kSampleAlphaToCoverage,
		kSampleAlphaToOne,
		kSampleCoverage,
		kSampleShading,
		kSampleMask,
		kScissorTest,
		kStencilTest,
		kTextureCubeMapSeamless,
		kProgramPointSize, kEnabledCapLast = kProgramPointSize
	};

	enum
	{
		kEnabledCapCount = kEnabledCapLast - kEnabledCapFirst + 1
	};

	enum ObjectType
	{
		kObjectTypeInvalid = 0xDEADDEAD,
		kBuffer = 0, kObjectTypeFirst = kBuffer,
		kShader,
		kProgram,
		kVertexArray,
		kQuery,
		kProgramPipeline,
		kTransformFeedback,
		kSampler,
		kTexture,
		kRenderbuffer,
		kFramebuffer, kObjectTypeLast = kFramebuffer
	};

	enum
	{
		kObjectTypeCount = kObjectTypeLast - kObjectTypeFirst + 1
	};

	enum QueryResult
	{
		kQueryResult = 0x8866,			// GL_QUERY_RESULT
		kQueryResultAvailable = 0x8867	// GL_QUERY_RESULT_AVAILABLE
	};

	enum BufferTarget
	{
		kBufferTargetInvalid = 0xDEADDEAD,
		kElementArrayBuffer = 0, kBufferTargetSingleBindingFirst = kElementArrayBuffer, kBufferTargetFirst = kElementArrayBuffer,
		kArrayBuffer,
		kCopyWriteBuffer,
		kCopyReadBuffer,
		kPixelPackBuffer,
		kPixelUnpackBuffer,
		kDispatchIndirectBuffer,
		kDrawIndirectBuffer,
		kParameterBuffer,
		kQueryBuffer, kBufferTargetSingleBindingLast = kQueryBuffer, // All the buffer targets before this point has a single binding point
		kUniformBuffer,
		kTransformFeedbackBuffer,
		kShaderStorageBuffer,
		kAtomicCounterBuffer, kBufferTargetLast = kAtomicCounterBuffer
	};

	enum
	{
		kBufferTargetCount = kBufferTargetLast - kBufferTargetFirst + 1,
		kBufferTargetSingleBindingCount = kBufferTargetSingleBindingLast - kBufferTargetSingleBindingFirst + 1
	};

	enum FramebufferRead
	{
		// We use kFramebufferReadDefault to not updated FBO glReadBuffer state and rely on the current object state
		kFramebufferReadNone = 0, kFramebufferReadFirst = kFramebufferReadNone,
		kFramebufferReadDefault,
		kFramebufferReadBack,
		kFramebufferReadColor0,
		kFramebufferReadColor1,
		kFramebufferReadColor2,
		kFramebufferReadColor3,
		kFramebufferReadColor4,
		kFramebufferReadColor5,
		kFramebufferReadColor6,
		kFramebufferReadColor7, kFramebufferReadLast = kFramebufferReadColor7
	};

	enum
	{
		kFramebufferReadCount = kFramebufferReadLast - kFramebufferReadFirst + 1
	};

	enum FramebufferTarget
	{
		kDrawFramebuffer = 0, kFramebufferTargetFirst = kDrawFramebuffer,
		kReadFramebuffer, kFramebufferTargetLast = kReadFramebuffer
	};

	enum
	{
		kFramebufferTargetCount = kFramebufferTargetLast - kFramebufferTargetFirst + 1
	};

	enum FramebufferMask
	{
		kFramebufferColorBit, kFramebufferMaskFirst = kFramebufferColorBit,
		kFramebufferDepthBit,
		kFramebufferStencilBit,
		kFramebufferColorDepthBit, kFramebufferMaskLast = kFramebufferColorDepthBit
	};

	enum
	{
		kFramebufferMaskCount = kFramebufferMaskLast + 1
	};

	enum MaxCaps
	{
		kMaxUniformBufferBindings				= 64,
		kMaxTransformFeedbackBufferBindings		= 4,
		kMaxShaderStorageBufferBindings			= 24, // Biggest known value supported (Adreno 420 on April 2015)
		kMaxAtomicCounterBufferBindings			= 8,
		kMaxTextureBindings						= 32
	};

	// GL TODO: This should be ApiGLES detail
	enum VertexArrayAttribKind
	{
		kVertexArrayAttribSNorm, kVertexArrayAttribFirst = kVertexArrayAttribSNorm,		// Floating values or normalized integer values
		kVertexArrayAttribSNormNormalize,												// Fixed-point data values will be normalized
		kVertexArrayAttribInteger,														// Integer values interpreted as integer by the shader
		kVertexArrayAttribLong, kVertexArrayAttribLast = kVertexArrayAttribLong			// Double values interpreted as double by the shader
	};

	enum
	{
		kVertexArrayAttribCount = kVertexArrayAttribLast - kVertexArrayAttribFirst + 1
	};

	enum VertexAttrLocation
	{
		kVertexAttrPosition = 0,
		kVertexAttrColor,
		kVertexAttrNormal,
		kVertexAttrTexCoord0,
		kVertexAttrTexCoord1,
		kVertexAttrTexCoord2,
		kVertexAttrTexCoord3,
		kVertexAttrTexCoord4,
		kVertexAttrTexCoord5,
		kVertexAttrTexCoord6,
		kVertexAttrTexCoord7, VertexAttrLast = kVertexAttrTexCoord7
	};

	enum
	{
		kVertexAttrCount = VertexAttrLast + 1
	};

	enum VertexArrayFlags
	{
		//Bit [1..0]: Called with glVertexAttrib*Pointer and normalized
		kPointerBits				= (0 << 0), // kVertexArrayAttribSNorm, glVertexAttribPointer
		kPointerNormalizedBits		= (1 << 0), // kVertexArrayAttribSnormNormalized, glVertexAttribPointer and normalized
		kPointerIBits				= (2 << 0), // kVertexArrayAttribInteger, glVertexAttribIPointer
		kPointerLBits				= (3 << 0), // kVertexArrayAttribLong, glVertexAttribLPointer
		//Bit [3..2]: size
		kVec1Bits					= (0 << 2), // size = 1
		kVec2Bits					= (1 << 2), // size = 2
		kVec3Bits					= (2 << 2), // size = 3
		kVec4Bits					= (3 << 2), // size = 4
		//Bit [6..4]: type
		kF32Bits					= (0 << 4), // type = GL_FLOAT,
		kF16Bits					= (1 << 4), // type = GL_HALF_FLOAT_OES or GL_HALF_FLOAT,
		kI8Bits						= (2 << 4), // type = GL_BYTE,
		kU8Bits						= (3 << 4), // type = GL_UNSIGNED_BYTE,
		kU32Bits					= (4 << 4), // type = GL_UNSIGNED_INT
	};

	enum ShaderStage
	{
		kVertexShaderStage = 0, kShaderStageFirst = kVertexShaderStage,
		kControlShaderStage,
		kEvalShaderStage,
		kGeometryShaderStage,
		kFragmentShaderStage,
		kComputeShaderStage, kShaderStageLast = kComputeShaderStage
	};

	enum
	{
		kShaderStageCount = kShaderStageLast - kShaderStageFirst + 1
	};

	enum MemoryBarrierType
	{
		kBarrierVertexAttribArray = 0, kBarrierTypeFirst = kBarrierVertexAttribArray,
		kBarrierElementArray,
		kBarrierUniform,
		kBarrierTextureFetch,
		kBarrierShaderImageAccess,
		kBarrierCommand,
		kBarrierPixelBuffer,
		kBarrierTextureUpdate,
		kBarrierBufferUpdate,
		kBarrierFramebuffer,
		kBarrierTransformFeedback,
		kBarrierAtomicCounter,
		kBarrierShaderStorage, kBarrierTypeLast = kBarrierShaderStorage
	};

	enum
	{
		kBarrierTypeCount = kBarrierTypeLast - kBarrierTypeFirst + 1
	};

	enum TextureSwizzleMode
	{
		kTextureSwizzleModeNone,
		kTextureSwizzleModeFormat,
		kTextureSwizzleModeConfigurable
	};

	enum TextureSwizzleFunc
	{
		kTextureSwizzleFuncNone = 0, kTextureSwizzleFuncFirst = kTextureSwizzleFuncNone,
		kTextureSwizzleFuncRZZO,
		kTextureSwizzleFuncRGZO,
		kTextureSwizzleFuncRGBO,
		kTextureSwizzleFuncRGBA,
		kTextureSwizzleFuncBGRO,
		kTextureSwizzleFuncBGRA,
		kTextureSwizzleFuncZZZR,
		kTextureSwizzleFuncARGB, kTextureSwizzleFuncLast = kTextureSwizzleFuncARGB
	};

	enum
	{
		kTextureSwizzleFuncCount = kTextureSwizzleFuncLast - kTextureSwizzleFuncFirst + 1
	};

	enum TextureSrgbDecode
	{
		kTextureSrgbDecode = 0, kTextureSrgbDecodeFirst = kTextureSrgbDecode,
		kTextureSrgbSkipDecode, kTextureSrgbDecodeLast = kTextureSrgbSkipDecode
	};

	enum
	{
		kTextureSrgbCount = kTextureSrgbDecodeLast - kTextureSrgbDecodeFirst + 1
	};

	enum TextureCap
	{
		kTextureCapCompressedBit = (1 << 0),
		kTextureCapETCBit = (1 << 1), // Implies ETC1, ETC2 and EAC
		kTextureCapStorage = (1 << 2),
		kTextureCapDepth = (1 << 3),
		kTextureCapStencil = (1 << 4),
		kTextureCapFloat = (1 << 5),
		kTextureCapHalf = (1 << 6),
		kTextureCapInteger = (1 << 7)
	};

	// Texture, colorbuffer and depthbuffer format supported by the OpenGL ES back-end
	// This is also used to translate from Unity formats to OpenGL formats supported by the platform
	// It allows creating OpenGL texture objects for both Unity texture and Unity render texture using the same code path in the back-end.
	enum TexFormat
	{
		kTexFormatNone, kTexFormatFirst = kTexFormatNone,
		kTexFormatRGB8unorm,
		kTexFormatRGB8srgb,
		kTexFormatRGBA8unorm,
		kTexFormatRGBA8srgb,
		kTexFormatBGR8unorm,
		kTexFormatBGR8srgb,
		kTexFormatBGRA8unorm,
		kTexFormatBGRA8srgb,
		kTexFormatARGB8unorm,
		kTexFormatARGB8srgb,
		kTexFormatRGB10A2unorm,
		kTexFormatR8unorm,
		kTexFormatA8unorm,
		kTexFormatA16unorm,
		kTexFormatR5G6B5unorm,
		kTexFormatRGB5A1unorm,
		kTexFormatRGBA4unorm,
		kTexFormatBGRA4unorm,
		kTexFormatR16sf,
		kTexFormatRG16sf,
		kTexFormatRGBA16sf,
		kTexFormatR32sf,
		kTexFormatRG32sf,
		kTexFormatRGB32sf,
		kTexFormatRGBA32sf,
		kTexFormatRG11B10uf,
		kTexFormatR32i,
		kTexFormatRG32i,
		kTexFormatRGBA32i,
		kTexFormatDepth16,
		kTexFormatDepth24,
		kTexFormatStencil8,
		kTexFormatCoverage4,
		kTexFormatRGB_DXT1unorm,
		kTexFormatRGB_DXT1srgb,
		kTexFormatRGBA_DXT3unorm,
		kTexFormatRGBA_DXT3srgb,
		kTexFormatRGBA_DXT5unorm,
		kTexFormatRGBA_DXT5srgb,
		kTexFormatRGB_PVRTC2BPPunorm,
		kTexFormatRGB_PVRTC2BPPsrgb,
		kTexFormatRGB_PVRTC4BPPunorm,
		kTexFormatRGB_PVRTC4BPPsrgb,
		kTexFormatRGBA_PVRTC2BPPunorm,
		kTexFormatRGBA_PVRTC2BPPsrgb,
		kTexFormatRGBA_PVRTC4BPPunorm,
		kTexFormatRGBA_PVRTC4BPPsrgb,
		kTexFormatRGB_ATCunorm,
		kTexFormatRGBA_ATCunorm,
		kTexFormatRGB_ETCunorm,
		kTexFormatRGB_ETCsrgb, // ETC1 sRGB does not exist in any OpenGL spec or extension, but we can treat the data as GL_COMPRESSED_SRGB8_ETC2 if available
		kTexFormatRGB_ETC2unorm,
		kTexFormatRGB_ETC2srgb,
		kTexFormatRGB_A1_ETC2unorm,
		kTexFormatRGB_A1_ETC2srgb,
		kTexFormatRGBA_ETC2unorm,
		kTexFormatRGBA_ETC2srgb,
		kTexFormatR_EACunorm,
		kTexFormatR_EACsnorm,
		kTexFormatRG_EACunorm,
		kTexFormatRG_EACsnorm,
		kTexFormatRGBA_ASTC4X4unorm,
		kTexFormatRGBA_ASTC4X4srgb,
		kTexFormatRGBA_ASTC5X5unorm,
		kTexFormatRGBA_ASTC5X5srgb,
		kTexFormatRGBA_ASTC6X6unorm,
		kTexFormatRGBA_ASTC6X6srgb,
		kTexFormatRGBA_ASTC8X8unorm,
		kTexFormatRGBA_ASTC8X8srgb,
		kTexFormatRGBA_ASTC10X10unorm,
		kTexFormatRGBA_ASTC10X10srgb,
		kTexFormatRGBA_ASTC12X12unorm,
		kTexFormatRGBA_ASTC12X12srgb, kTexFormatLast = kTexFormatRGBA_ASTC12X12srgb
	};

	enum
	{
		kTexFormatCount = kTexFormatLast - kTexFormatFirst + 1,
		kTexFormatInvalid = -1
	};

	bool IsFormatDepthStencil(gl::TexFormat texFormat);
	bool IsFormatFloat(gl::TexFormat texFormat);
	bool IsFormatHalf(gl::TexFormat texFormat);
	bool IsFormatIEEE754(gl::TexFormat texFormat);
	bool IsFormatInteger(gl::TexFormat texFormat);

}//namespace gl
