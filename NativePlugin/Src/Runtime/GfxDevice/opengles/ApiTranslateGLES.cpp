//////////////////////////////////////////////////////////////////////////
// Design direction for the translation tables
//
// All translation tables which content may change because of different
// levels when recreating the GfxDeviceGLES needs to be define const 
// instead of static const because static variables are initialized for
// life in C++.
//
// However, when initializing a variable/array const, compilers translate
// the code into a long series of instruction transforming a simple and
// fast table indexing into a costly execution prone for instruction cache
// evictions.
//
// As a result, it' ok-ish to declare translation tables in
// TranslateGLES::Init*** function const only because those are called only
// as GfxdeviceGLES creation but all the translation query functions
// should use static const tables or accessing initialized tables.
//////////////////////////////////////////////////////////////////////////

#include "UnityPrefix.h"

#include "ApiFuncGLES.h"
#include "ApiTranslateGLES.h"
#include "ApiConstantsGLES.h"
#include "GraphicsCapsGLES.h"
#include "Runtime/Shaders/GraphicsCaps.h"
#include "Runtime/Utilities/ArrayUtility.h"

#if FORCE_DEBUG_BUILD_WEBGL
#	undef UNITY_WEBGL
#	define UNITY_WEBGL 1
#endif//FORCE_DEBUG_BUILD_WEBGL

namespace
{
	// -- ES3 --
	const GLenum GL_ETC1_RGB8_OES								= 0x8D64;
	const GLenum GL_COMPRESSED_R11_EAC							= 0x9270;
	const GLenum GL_COMPRESSED_SIGNED_R11_EAC					= 0x9271;
	const GLenum GL_COMPRESSED_RG11_EAC							= 0x9272;
	const GLenum GL_COMPRESSED_SIGNED_RG11_EAC					= 0x9273;
	const GLenum GL_COMPRESSED_RGB8_ETC2						= 0x9274;
	const GLenum GL_COMPRESSED_SRGB8_ETC2						= 0x9275;
	const GLenum GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2	= 0x9276;
	const GLenum GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2	= 0x9277;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC			= 0x9279;

	// -- ASTC --
	const GLenum GL_COMPRESSED_RGBA_ASTC_4x4					= 0x93B0;
	const GLenum GL_COMPRESSED_RGBA_ASTC_5x5					= 0x93B2;
	const GLenum GL_COMPRESSED_RGBA_ASTC_6x6					= 0x93B4;
	const GLenum GL_COMPRESSED_RGBA_ASTC_8x8					= 0x93B7;
	const GLenum GL_COMPRESSED_RGBA_ASTC_10x10					= 0x93BB;
	const GLenum GL_COMPRESSED_RGBA_ASTC_12x12					= 0x93BD;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4			= 0x93D0;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5			= 0x93D2;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6			= 0x93D4;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8			= 0x93D7;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10			= 0x93DB;
	const GLenum GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12			= 0x93DD;

	// -- DXT --
	const GLenum GL_COMPRESSED_RGB_S3TC_DXT1_EXT				= 0x83F0;
	const GLenum GL_COMPRESSED_RGBA_S3TC_DXT1_EXT				= 0x83F1;
	const GLenum GL_COMPRESSED_RGBA_S3TC_DXT3_EXT				= 0x83F2;
	const GLenum GL_COMPRESSED_RGBA_S3TC_DXT5_EXT				= 0x83F3;
	const GLenum GL_COMPRESSED_SRGB_S3TC_DXT1_NV				= 0x8C4C;
	const GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_NV			= 0x8C4D;
	const GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_NV			= 0x8C4E;
	const GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_NV			= 0x8C4F;

	// -- PVRTC --
	const GLenum GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG				= 0x8C01;
	const GLenum GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG				= 0x8C00;
	const GLenum GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG			= 0x8C03;
	const GLenum GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG			= 0x8C02;
	const GLenum GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT			= 0x8A54;
	const GLenum GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT			= 0x8A55;
	const GLenum GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT		= 0x8A56;
	const GLenum GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT		= 0x8A57;

	// -- ATC --
	const GLenum GL_ATC_RGB_AMD									= 0x8C92;
	const GLenum GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD				= 0x87EE;

	// -- Uncompressed internal formats
	const GLenum GL_RGB8									= 0x8051;
	const GLenum GL_RG8										= 0x822B;
	const GLenum GL_R8										= 0x8229;
	const GLenum GL_RGBA32F									= 0x8814;
	const GLenum GL_RGB32F									= 0x8815;
	const GLenum GL_RG32F									= 0x8230;
	const GLenum GL_R32F									= 0x822E;
	const GLenum GL_RGBA16F									= 0x881A;
	const GLenum GL_RGB16F									= 0x881B;
	const GLenum GL_RG16F									= 0x822F;
	const GLenum GL_R16F									= 0x822D;
	const GLenum GL_RGB10_A2								= 0x8059;
	const GLenum GL_ALPHA8									= 0x803C;
	const GLenum GL_ALPHA16									= 0x803E;
	const GLenum GL_R16										= 0x822A;
	const GLenum GL_BGRA8_EXT								= 0x93A1;
	const GLenum GL_RGBA4									= 0x8056;
	const GLenum GL_RGB5_A1									= 0x8057;
	const GLenum GL_RGB565									= 0x8D62;
	const GLenum GL_SRGB8									= 0x8C41;
	const GLenum GL_R11F_G11F_B10F							= 0x8C3A;
	const GLenum GL_DEPTH_COMPONENT16_NONLINEAR_NV			= 0x8E2C;
	const GLenum GL_DEPTH32F_STENCIL8						= 0x8CAD;

	// -- Unsized internal formats --
	const GLenum GL_SRGB									= 0x8C40;
	const GLenum GL_SRGB_ALPHA_EXT							= 0x8C42;

	// -- Types --
	const GLenum GL_UNSIGNED_INT_24_8						= 0x84FA;
	const GLenum GL_UNSIGNED_INT_2_10_10_10_REV				= 0x8368;
	const GLenum GL_UNSIGNED_INT_10F_11F_11F_REV			= 0x8C3B;

	// -- For EXT_debug_label (~ES) --
	const GLenum GL_BUFFER_OBJECT_EXT						= 0x9151;
	const GLenum GL_SHADER_OBJECT_EXT						= 0x8B48;
	const GLenum GL_PROGRAM_OBJECT_EXT						= 0x8B40;
	const GLenum GL_VERTEX_ARRAY_OBJECT_EXT					= 0x9154;
	const GLenum GL_QUERY_OBJECT_EXT						= 0x9153;
	const GLenum GL_PROGRAM_PIPELINE_OBJECT_EXT				= 0x8A4F;

	// -- GL_EXT_texture_sRGB_decode --
	const GLenum GL_DECODE_EXT								= 0x8A49;
	const GLenum GL_SKIP_DECODE_EXT							= 0x8A4A;

	gl::TexFormat MakeSRGB(gl::TexFormat texFormat)
	{
		static const gl::TexFormat table[] = // kTexFormatCount
		{
			gl::kTexFormatNone,
			gl::kTexFormatRGB8srgb, gl::kTexFormatRGB8srgb,
			gl::kTexFormatRGBA8srgb, gl::kTexFormatRGBA8srgb,
			gl::kTexFormatBGR8srgb, gl::kTexFormatBGR8srgb,
			gl::kTexFormatBGRA8srgb, gl::kTexFormatBGRA8srgb,
			gl::kTexFormatARGB8srgb, gl::kTexFormatARGB8srgb,
			gl::kTexFormatRGB10A2unorm,
			gl::kTexFormatR8unorm, gl::kTexFormatA8unorm, gl::kTexFormatA16unorm,
			gl::kTexFormatR5G6B5unorm, gl::kTexFormatRGB5A1unorm, gl::kTexFormatRGBA4unorm, gl::kTexFormatBGRA4unorm,
			gl::kTexFormatR16sf, gl::kTexFormatRG16sf, gl::kTexFormatRGBA16sf,
			gl::kTexFormatR32sf, gl::kTexFormatRG32sf, gl::kTexFormatRGB32sf, gl::kTexFormatRGBA32sf,
			gl::kTexFormatRG11B10uf,
			gl::kTexFormatR32i, gl::kTexFormatRG32i, gl::kTexFormatRGBA32i,
			gl::kTexFormatDepth16, gl::kTexFormatDepth24, gl::kTexFormatStencil8, gl::kTexFormatCoverage4,
			gl::kTexFormatRGB_DXT1srgb, gl::kTexFormatRGB_DXT1srgb,
			gl::kTexFormatRGBA_DXT3srgb, gl::kTexFormatRGBA_DXT3srgb,
			gl::kTexFormatRGBA_DXT5srgb, gl::kTexFormatRGBA_DXT5srgb,
			gl::kTexFormatRGB_PVRTC2BPPsrgb, gl::kTexFormatRGB_PVRTC2BPPsrgb,
			gl::kTexFormatRGB_PVRTC4BPPsrgb, gl::kTexFormatRGB_PVRTC4BPPsrgb,
			gl::kTexFormatRGBA_PVRTC2BPPsrgb, gl::kTexFormatRGBA_PVRTC2BPPsrgb,
			gl::kTexFormatRGBA_PVRTC4BPPsrgb, gl::kTexFormatRGBA_PVRTC4BPPsrgb,
			gl::kTexFormatRGB_ATCunorm, gl::kTexFormatRGBA_ATCunorm,
			gl::kTexFormatRGB_ETCsrgb, gl::kTexFormatRGB_ETCsrgb,
			gl::kTexFormatRGB_ETC2srgb, gl::kTexFormatRGB_ETC2srgb,
			gl::kTexFormatRGB_A1_ETC2srgb, gl::kTexFormatRGB_A1_ETC2srgb,
			gl::kTexFormatRGBA_ETC2srgb, gl::kTexFormatRGBA_ETC2srgb,
			gl::kTexFormatR_EACunorm, gl::kTexFormatR_EACsnorm, gl::kTexFormatRG_EACunorm, gl::kTexFormatRG_EACsnorm,
			gl::kTexFormatRGBA_ASTC4X4srgb, gl::kTexFormatRGBA_ASTC4X4srgb,
			gl::kTexFormatRGBA_ASTC5X5srgb, gl::kTexFormatRGBA_ASTC5X5srgb,
			gl::kTexFormatRGBA_ASTC6X6srgb, gl::kTexFormatRGBA_ASTC6X6srgb,
			gl::kTexFormatRGBA_ASTC8X8srgb, gl::kTexFormatRGBA_ASTC8X8srgb,
			gl::kTexFormatRGBA_ASTC10X10srgb, gl::kTexFormatRGBA_ASTC10X10srgb,
			gl::kTexFormatRGBA_ASTC12X12srgb, gl::kTexFormatRGBA_ASTC12X12srgb
		};
		CompileTimeAssertArraySize(table, gl::kTexFormatCount);

		return table[texFormat];
	}
}//namespace

const GLenum TranslateGLES::kInvalidEnum = static_cast<GLenum>(0xBEEFBEEF);

void TranslateGLES::Init(const GraphicsCaps & caps, GfxDeviceLevelGL level)
{
	this->InitTextureFormat2Format(caps);
	this->InitDepthBufferFormat2Format(caps);
	this->InitRenderTextureFormat2Format(caps);
	this->InitFormat(caps);
	this->InitTextureTarget(caps);
	this->InitVertexType(caps, level);
	this->InitBufferTarget(caps, level);
	this->InitObjectType(caps);
	this->InitFramebufferTarget(caps);
}

// -- textureDimension to OpenGL enum translation --

GLenum TranslateGLES::GetTextureSRGBDecode(gl::TextureSrgbDecode srgbDecode) const
{
	static const GLenum table[] =
	{
		GL_DECODE_EXT,									// kTextureSrgbDecode
		GL_SKIP_DECODE_EXT,								// kTextureSrgbSkipDecode
	};
	CompileTimeAssertArraySize(table, gl::kTextureSrgbCount);

	GLenum translated = table[srgbDecode - gl::kTextureSrgbDecodeFirst];
	AssertFormatMsg(translated != kInvalidEnum, "OPENGL ERROR: TranslateGLES::GetTextureSRGBDecode - Invalid output for input %d", srgbDecode);

	return translated;
}

void TranslateGLES::InitTextureTarget(const GraphicsCaps & caps)
{
	const GLenum translation[] =
	{
		GL_TEXTURE_2D,																	// kTexDim2D
		caps.has3DTexture ? GL_TEXTURE_3D : kInvalidEnum,								// kTexDim3D
		GL_TEXTURE_CUBE_MAP,															// kTexDimCUBE
		caps.has2DArrayTexture ? GL_TEXTURE_2D_ARRAY : kInvalidEnum,					// kTexDim2DArray
	};
	CompileTimeAssertArraySize(translation, kTexDimLast - kTexDimFirst + 1);

	std::copy(&translation[0], &translation[0] + ARRAY_SIZE(translation), m_TextureTarget.begin());
}

GLenum TranslateGLES::GetTextureTarget(TextureDimension textureDimension) const
{
	AssertFormatMsg(textureDimension >= kTexDimFirst && textureDimension <= kTexDimLast, "OPENGL ERROR: TranslateGLES::TextureTarget - Invalid input: %d", textureDimension);

	GLenum translated = m_TextureTarget[textureDimension - kTexDimFirst];
	AssertFormatMsg(translated != kInvalidEnum, "OPENGL ERROR: TranslateGLES::GetTextureTarget - Invalid output for input %d", textureDimension);

	return translated;
}

const GLint* TranslateGLES::GetTextureSwizzle(gl::TextureSwizzleFunc func) const
{
	static const GLint translation[] = // kTextureSwizzleFuncCount * 4
	{
		GL_ZERO, GL_ZERO, GL_ZERO, GL_ONE,		// kTextureSwizzleFuncNone
		GL_RED, GL_ZERO, GL_ZERO, GL_ONE,		// kTextureSwizzleFuncRZZO
		GL_RED, GL_GREEN, GL_ZERO, GL_ONE,		// kTextureSwizzleFuncRGZO
		GL_RED, GL_GREEN, GL_BLUE, GL_ONE,		// kTextureSwizzleFuncRGBO
		GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA,	// kTextureSwizzleFuncRGBA
		GL_BLUE, GL_GREEN, GL_RED, GL_ONE,		// kTextureSwizzleFuncBGRO
		GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA,	// kTextureSwizzleFuncBGRA
		GL_ZERO, GL_ZERO, GL_ZERO, GL_RED,		// kTextureSwizzleFuncZZZR
		GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE,	// kTextureSwizzleFuncARGB
	};
	CompileTimeAssertArraySize(translation, gl::kTextureSwizzleFuncCount * 4);

	return &translation[func * 4];
}

TextureDimension TranslateGLES::GetTextureTarget(GLenum target) const
{
	for (int index = 0; index < kTexDimActiveCount; ++index)
		if (m_TextureTarget[index] == target)
			return static_cast<TextureDimension>(index + kTexDimFirst);

	DebugAssertMsg(0, "OPENGL ERROR: TranslateGLES::GetTextureTarget - Invalid input enum");
	return kTexDimUnknown;
}

// OpenGL ES format to gl::TexFormat
// When the users create textures using OpenGL formats,
// these formats are not necessarily cross platforms or between OpenGL versions
// For example GL_SRGB_ALPHA_EXT is only value with OpenGL ES 2.0 with texture images
// On OpenGL ES 3.0 or texture storage, this will generate an OpenGL error.
// Converting these values to gl::TexFormat we can use the correct OpenGL formats in a cross platform manner
// relying on the ApiTranslateGLES to figure out according to the caps what's the platform and OpenGL API wants.

gl::TexFormat TranslateGLES::GetFormatGLES(GLenum glesFormat) const
{
	switch(glesFormat)
	{
	default:				Assert(false && "unsupported texture format"); return gl::kTexFormatRGBA8unorm;
	case GL_RGB8:										return gl::kTexFormatRGB8unorm;
	case GL_SRGB8:										return gl::kTexFormatRGB8srgb;
	case GL_RGBA8:										return gl::kTexFormatRGBA8unorm;
	case GL_SRGB8_ALPHA8:								return gl::kTexFormatRGBA8srgb;
	case GL_SRGB_ALPHA_EXT:								return gl::kTexFormatRGBA8srgb;
	case GL_BGRA8_EXT:									return gl::kTexFormatBGRA8unorm;
	case GL_RGBA4:										return gl::kTexFormatRGBA4unorm;
	case GL_RGB565:										return gl::kTexFormatR5G6B5unorm;
	case GL_RGB5_A1:									return gl::kTexFormatRGB5A1unorm;
	case GL_ALPHA:										return gl::kTexFormatA8unorm;
	case GL_ALPHA8:										return gl::kTexFormatA8unorm;
	case GL_ALPHA16:									return gl::kTexFormatA16unorm;
	case GL_R16F:										return gl::kTexFormatR16sf;
	case GL_RG16F:										return gl::kTexFormatRG16sf;
	case GL_RGBA16F:									return gl::kTexFormatRGBA16sf;
	case GL_R32F:										return gl::kTexFormatR32sf;
	case GL_RG32F:										return gl::kTexFormatRG32sf;
	case GL_RGB32F:										return gl::kTexFormatRGB32sf;
	case GL_RGBA32F:									return gl::kTexFormatRGBA32sf;
	case GL_R11F_G11F_B10F:								return gl::kTexFormatRG11B10uf;
	case GL_RGB10_A2:									return gl::kTexFormatRGB10A2unorm;
	case GL_R16:										return gl::kTexFormatR16sf;
	case GL_DEPTH_COMPONENT16:							return gl::kTexFormatDepth16;
	case GL_DEPTH_COMPONENT16_NONLINEAR_NV:				return gl::kTexFormatDepth16;
	case GL_DEPTH_COMPONENT24:							return gl::kTexFormatDepth24;
	case GL_DEPTH24_STENCIL8:							return gl::kTexFormatDepth24;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:				return gl::kTexFormatRGB_DXT1unorm;
	case GL_COMPRESSED_SRGB_S3TC_DXT1_NV:				return gl::kTexFormatRGB_DXT1srgb;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:				return gl::kTexFormatRGBA_DXT3unorm;
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_NV:			return gl::kTexFormatRGBA_DXT3srgb;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:				return gl::kTexFormatRGBA_DXT5unorm;
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_NV:			return gl::kTexFormatRGBA_DXT5srgb;
	case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:			return gl::kTexFormatRGB_PVRTC2BPPunorm;
	case GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT:			return gl::kTexFormatRGB_PVRTC2BPPsrgb;
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:			return gl::kTexFormatRGBA_PVRTC2BPPunorm;
	case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT:		return gl::kTexFormatRGBA_PVRTC2BPPsrgb;
	case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:			return gl::kTexFormatRGB_PVRTC4BPPunorm;
	case GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT:			return gl::kTexFormatRGB_PVRTC4BPPsrgb;
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:			return gl::kTexFormatRGBA_PVRTC4BPPunorm;
	case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT:		return gl::kTexFormatRGBA_PVRTC4BPPsrgb;
	case GL_ATC_RGB_AMD:								return gl::kTexFormatRGB_ATCunorm;
	case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:			return gl::kTexFormatRGBA_ATCunorm;
	case GL_COMPRESSED_R11_EAC:							return gl::kTexFormatR_EACunorm;
	case GL_COMPRESSED_SIGNED_R11_EAC:					return gl::kTexFormatR_EACsnorm;
	case GL_COMPRESSED_RG11_EAC:						return gl::kTexFormatRG_EACunorm;
	case GL_COMPRESSED_SIGNED_RG11_EAC:					return gl::kTexFormatRG_EACsnorm;
	case GL_ETC1_RGB8_OES:								return gl::kTexFormatRGB_ETCunorm;
	case GL_COMPRESSED_RGB8_ETC2:						return gl::kTexFormatRGB_ETC2unorm;
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:	return gl::kTexFormatRGB_A1_ETC2unorm;
	case GL_COMPRESSED_RGBA8_ETC2_EAC:					return gl::kTexFormatRGBA_ETC2unorm;
	case GL_COMPRESSED_RGBA_ASTC_4x4:					return gl::kTexFormatRGBA_ASTC4X4unorm;
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:			return gl::kTexFormatRGBA_ASTC4X4srgb;
	case GL_COMPRESSED_RGBA_ASTC_5x5:					return gl::kTexFormatRGBA_ASTC5X5unorm;
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5:			return gl::kTexFormatRGBA_ASTC5X5srgb;
	case GL_COMPRESSED_RGBA_ASTC_6x6:					return gl::kTexFormatRGBA_ASTC6X6unorm;
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6:			return gl::kTexFormatRGBA_ASTC6X6srgb;
	case GL_COMPRESSED_RGBA_ASTC_8x8:					return gl::kTexFormatRGBA_ASTC8X8unorm;
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8:			return gl::kTexFormatRGBA_ASTC8X8srgb;
	case GL_COMPRESSED_RGBA_ASTC_10x10:					return gl::kTexFormatRGBA_ASTC10X10unorm;
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10:			return gl::kTexFormatRGBA_ASTC10X10srgb;
	case GL_COMPRESSED_RGBA_ASTC_12x12:					return gl::kTexFormatRGBA_ASTC12X12unorm;
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12:			return gl::kTexFormatRGBA_ASTC12X12srgb;
	};
}

// Unified Format

void TranslateGLES::InitFormat(const GraphicsCaps & caps)
{
	const bool hasSRGB = caps.gles.hasTextureSrgb;
	const bool hasDXTSRGB = caps.gles.hasDxtSrgb;
	const bool hasPVRSRGB = caps.gles.hasPvrSrgb;

	// ES2, when texture storage isn't supported, only supports unsized formats
	const bool full = !IsGfxLevelES2(GetGraphicsCaps().gles.featureLevel) || GetGraphicsCaps().gles.hasTextureStorage;

	const GLenum internalRGB8 = full ? GL_RGB8 : GL_RGB;
	const GLenum internalRGBA8 = full ? GL_RGBA8 : GL_RGBA;
	const GLenum internalSRGB8 = hasSRGB ? (full ? GL_SRGB8 : GL_SRGB) : internalRGB8;
	const GLenum internalSRGB8_A8 = hasSRGB ? (full ? GL_SRGB8_ALPHA8 : GL_SRGB_ALPHA_EXT) : internalRGBA8;
	const GLenum externalSRGB8 = hasSRGB ? (full ? GL_RGB : GL_SRGB) : GL_RGB;
	const GLenum externalSRGB8_A8 = hasSRGB ? (full ? GL_RGBA : GL_SRGB_ALPHA_EXT) : GL_RGBA;

	// Core profile doesn't support alpha texture.
	// With GL 3.3+ and ES3.0+ we use red texture and texture swizzle to fetch from the alpha channel in the shader
	// GL3.2 convert the alpha texture into RGBA data hence this format doesn't apply
	// ES2 doesn't support RED formats and texture storage doesn't support alpha format. Use GL_ALPHA for this case.
	const GLenum internalAlpha8 = caps.gles.hasTextureSwizzle == gl::kTextureSwizzleModeConfigurable ? GL_R8 : GL_ALPHA;
	const GLenum externalAlpha8 = caps.gles.hasTextureSwizzle == gl::kTextureSwizzleModeConfigurable ? GL_RED : GL_ALPHA;

	// By default (ES 3.0 / GL 3.3) Swizzling is handled by texture swizzle to we use RGBA formats
	GLenum internalBGRA8 = internalRGBA8;
	GLenum internalSBGR8_A8 = internalSRGB8_A8;
	GLenum externalBGRA8 = GL_RGBA;

	// When texture swizzle isn't supported we fallback to swizzle formats (ES 2.0 and GL 3.2)
	if (GetGraphicsCaps().gles.hasTextureSwizzle == gl::kTextureSwizzleModeFormat)
	{
		if (IsGfxLevelCore(GetGraphicsCaps().gles.featureLevel))
		{
			internalBGRA8 = GL_RGBA8;
			externalBGRA8 = GL_BGRA;
		}
		else
		{
			// EXT_texture_format_BGRA8888 and APPLE_texture_format_BGRA8888 dictates internal format for GL_BGRA_EXT to be GL_RGBA
			// BUT in case of texture storage support, it must be GL_BGRA8_EXT
			internalBGRA8 = GetGraphicsCaps().gles.hasTextureStorage ? GL_BGRA8_EXT : GL_BGRA;
			externalBGRA8 = GL_BGRA;
		}
	}

	// ES2 OES_texture_float defines a different enum value for half float type than ES and desktop
	const GLenum typeHalf = IsGfxLevelES2(caps.gles.featureLevel) ? GL_HALF_FLOAT_OES : GL_HALF_FLOAT;

	// RGB8 ETC1 and RGB8 ETC2 are the same formats except that the first enum is only used for a ES2 extension
	const GLenum internalETC1 = IsGfxLevelES2(caps.gles.featureLevel) ? GL_ETC1_RGB8_OES : GL_COMPRESSED_RGB8_ETC2;
	const GLenum internalETC1srgb = IsGfxLevelES2(caps.gles.featureLevel) ? GL_ETC1_RGB8_OES : GL_COMPRESSED_SRGB8_ETC2;

	// Initalize per texture format caps
	const GLuint capStorage = GetGraphicsCaps().gles.hasTextureStorage ? gl::kTextureCapStorage : 0;
	const GLuint capCompressed = capStorage | gl::kTextureCapCompressedBit;

	// Texture storage isn't supported with (ES2 only) alpha textures
	const GLuint capAlpha = !IsGfxLevelES2(caps.gles.featureLevel) && GetGraphicsCaps().gles.hasTextureStorage && GetGraphicsCaps().gles.hasTextureSwizzle ? gl::kTextureCapStorage : 0;

	// See buggyTexStorageNonSquareMipMapETC declaration comment
	const GLuint capETC = gl::kTextureCapETCBit | gl::kTextureCapCompressedBit | (g_GraphicsCapsGLES->buggyTexStorageNonSquareMipMapETC ? 0 : capStorage);
	const GLuint capDXT = gl::kTextureCapCompressedBit | (g_GraphicsCapsGLES->buggyTexStorageDXT ? 0 : capStorage);

	const GLuint capInteger = capStorage | gl::kTextureCapInteger;
	const GLuint capHalf = capStorage | gl::kTextureCapHalf;
	const GLuint capFloat = capStorage | gl::kTextureCapFloat;
	const GLuint capDepth = capStorage | gl::kTextureCapDepth;
	const GLuint capDepthStencil = capStorage | capDepth | (GetGraphicsCaps().hasStencil ? gl::kTextureCapStencil : 0);
	const GLuint capStencil = capStorage | (GetGraphicsCaps().hasStencil ? gl::kTextureCapStencil : 0);

	const GLenum sRGBDXT1 = hasDXTSRGB ? GL_COMPRESSED_SRGB_S3TC_DXT1_NV : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	const GLenum sRGBDXT3 = hasDXTSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_NV : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	const GLenum sRGBDXT5 = hasDXTSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_NV : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	const GLenum sRGBPVRTC_2BPP = hasPVRSRGB ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
	const GLenum sRGBAPVRTC_2BPP = hasPVRSRGB ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
	const GLenum sRGBPVRTC_4BPP = hasPVRSRGB ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
	const GLenum sRGBAPVRTC_4BPP = hasPVRSRGB ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;

	// Depth formats
	const GLenum internalDepth16 = caps.gles.hasNVNLZ ? GL_DEPTH_COMPONENT16_NONLINEAR_NV : GL_DEPTH_COMPONENT16;
	const GLenum internalDepthStencil = UNITY_WEBGL && IsGfxLevelES2(GetGraphicsCaps().gles.featureLevel) ? GL_DEPTH_STENCIL : GL_DEPTH24_STENCIL8;
	
	GLenum internalDepth24 = internalDepth16;
	GLenum externalDepth24 = GL_DEPTH_COMPONENT;
	GLenum typeDepth24 = GL_UNSIGNED_SHORT;
	if (GetGraphicsCaps().gles.hasPackedDepthStencil)
	{
		internalDepth24 = internalDepthStencil;
		externalDepth24 = GL_DEPTH_STENCIL;
		typeDepth24 = GL_UNSIGNED_INT_24_8;
	}
	else if (GetGraphicsCaps().gles.hasDepth24)
	{
		internalDepth24 = GL_DEPTH_COMPONENT24;
		typeDepth24 = GL_UNSIGNED_BYTE;
	}

	// See declaration of TextureFormatGLES for a description of each member
	const FormatDescGLES table[] = // kFormatCount
	{
		//	GLenum internalFormat						GLenum externalFormat		GLenum type						GLuint flags	TextureSwizzleFunc				blockSize	// gl::TexFormat
		{GL_NONE,										GL_NONE,					GL_NONE,						0,				gl::kTextureSwizzleFuncNone,	0},			// kTexFormatNone
		{GL_RGB8,										GL_RGB,						GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncRGBO,	3},			// kTexFormatRGB8unorm
		{internalSRGB8,									externalSRGB8,				GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncRGBO,	3},			// kTexFormatRGB8srgb
		{GL_RGBA8,										GL_RGBA,					GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncRGBA,	4},			// kTexFormatRGBA8unorm
		{internalSRGB8_A8,								externalSRGB8_A8,			GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncRGBA,	4},			// kTexFormatRGBA8srgb
		{GL_RGB8,										GL_RGB,						GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncBGRO,	3},			// kTexFormatBGR8unorm
		{internalSRGB8,									externalSRGB8,				GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncBGRO,	3},			// kTexFormatBGR8srgb
		{internalBGRA8,									externalBGRA8,				GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncBGRA,	4},			// kTexFormatBGRA8unorm
		{internalSBGR8_A8,								externalBGRA8,				GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncBGRA,	4},			// kTexFormatBGRA8srgb
		{GL_RGBA8,										GL_RGBA,					GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncARGB,	4},			// kTexFormatARGB8unorm
		{full ? GL_SRGB8_ALPHA8 : GL_SRGB_ALPHA_EXT,	GL_RGBA,					GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncARGB,	4},			// kTexFormatARGB8srgb
		{GL_RGB10_A2,									GL_RGBA,					GL_UNSIGNED_INT_2_10_10_10_REV,	capStorage,		gl::kTextureSwizzleFuncRGBA,	4},			// kTexFormatRGB10A2unorm
		{GL_R8,											GL_RED,						GL_UNSIGNED_BYTE,				capStorage,		gl::kTextureSwizzleFuncRZZO,	1},			// kTexFormatR8unorm
		{internalAlpha8,								externalAlpha8,				GL_UNSIGNED_BYTE,				capAlpha,		gl::kTextureSwizzleFuncZZZR,	1},			// kTexFormatA8unorm
		{GL_R16,										GL_RED,						GL_UNSIGNED_SHORT,				capStorage,		gl::kTextureSwizzleFuncZZZR,	2},			// kTexFormatA16unorm
		{GL_RGB565,										GL_RGB,						GL_UNSIGNED_SHORT_5_6_5,		capStorage,		gl::kTextureSwizzleFuncRGBO,	2},			// kTexFormatR5G6B5unorm
		{GL_RGB5_A1,									GL_RGBA,					GL_UNSIGNED_SHORT_5_5_5_1,		capStorage,		gl::kTextureSwizzleFuncRGBA,	2},			// kTexFormatRGB5A1unorm
		{GL_RGBA4,										GL_RGBA,					GL_UNSIGNED_SHORT_4_4_4_4,		capStorage,		gl::kTextureSwizzleFuncRGBA,	2},			// kTexFormatRGBA4unorm
		{GL_RGBA4,										GL_RGBA,					GL_UNSIGNED_SHORT_4_4_4_4,		capStorage,		gl::kTextureSwizzleFuncBGRA,	2},			// kTexFormatBGRA4unorm
		{GL_R16F,										GL_RED,						typeHalf,						capHalf,		gl::kTextureSwizzleFuncRZZO,	2},			// kTexFormatR16sf
		{GL_RG16F,										GL_RG,						typeHalf,						capHalf,		gl::kTextureSwizzleFuncRGZO,	4},			// kTexFormatRGHalf
		{GL_RGBA16F,									GL_RGBA,					typeHalf,						capHalf,		gl::kTextureSwizzleFuncRGBA,	8},			// kTexFormatRGBAHalf
		{GL_R32F,										GL_RED,						GL_FLOAT,						capFloat,		gl::kTextureSwizzleFuncRZZO,	4},			// kTexFormatR32sf
		{GL_RG32F,										GL_RG,						GL_FLOAT,						capFloat,		gl::kTextureSwizzleFuncRGZO,	8},			// kTexFormatRG32sf
		{GL_RGB32F,										GL_RGB,						GL_FLOAT,						capFloat,		gl::kTextureSwizzleFuncRGBO,	12},		// kTexFormatRGB32sf
		{GL_RGBA32F,									GL_RGBA,					GL_FLOAT,						capFloat,		gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA32sf
		{GL_R11F_G11F_B10F,								GL_RGB,						GL_UNSIGNED_INT_10F_11F_11F_REV,capStorage,		gl::kTextureSwizzleFuncRGBO,	4},			// kTexFormatR11G11B10f
		{GL_R32I,										GL_RED_INTEGER,				GL_INT,							capInteger,		gl::kTextureSwizzleFuncRZZO,	4},			// kTexFormatR32i
		{GL_RG32I,										GL_RG_INTEGER,				GL_INT,							capInteger,		gl::kTextureSwizzleFuncRGZO,	8},			// kTexFormatRG32i
		{GL_RGBA32I,									GL_RGBA_INTEGER,			GL_INT,							capInteger,		gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA32i
		{internalDepth16,								GL_DEPTH_COMPONENT,			GL_UNSIGNED_SHORT,				capDepth,		gl::kTextureSwizzleFuncRGBA,	2},			// kTexFormatDepth16
		{internalDepth24,								externalDepth24,			typeDepth24,					capDepthStencil,gl::kTextureSwizzleFuncRGBA,	3},			// kTexFormatDepth24
		{GL_STENCIL_INDEX8,								GL_NONE,					GL_NONE,						capStencil,		gl::kTextureSwizzleFuncRGBA,	1},			// kTexFormatStencil8
		{GL_COVERAGE_COMPONENT4_NV,						GL_NONE,					GL_NONE,						capStorage,		gl::kTextureSwizzleFuncRGBA,	0},			// kTexFormatCoverage4
		{GL_COMPRESSED_RGB_S3TC_DXT1_EXT,				GL_NONE,					GL_NONE,						capDXT,			gl::kTextureSwizzleFuncRGBO,	8},			// kTexFormatRGB_DXT1unorm
		{sRGBDXT1,										GL_NONE,					GL_NONE,						capDXT,			gl::kTextureSwizzleFuncRGBO,	8},			// kTexFormatRGB_DXT1srgb
		{GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,				GL_NONE,					GL_NONE,						capDXT,			gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_DXT3unorm
		{sRGBDXT3,										GL_NONE,					GL_NONE,						capDXT,			gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_DXT3srgb
		{GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,				GL_NONE,					GL_NONE,						capDXT,			gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_DXT5unorm
		{sRGBDXT5,										GL_NONE,					GL_NONE,						capDXT,			gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_DXT5srgb
		{GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBO,	32},		// kTexFormatRGB_PVRTC2BPPunorm
		{sRGBPVRTC_2BPP,								GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBO,	32},		// kTexFormatRGB_PVRTC2BPPsrgb
		{GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBO,	32},		// kTexFormatRGB_PVRTC4BPPunorm
		{sRGBPVRTC_4BPP,								GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBO,	32},		// kTexFormatRGB_PVRTC4BPPsrgb
		{GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	32},		// kTexFormatRGBA_PVRTC2BPPunorm
		{sRGBAPVRTC_2BPP,								GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	32},		// kTexFormatRGBA_PVRTC2BPPsrgb
		{GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	32},		// kTexFormatRGBA_PVRTC4BPPunorm
		{sRGBAPVRTC_4BPP,								GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	32},		// kTexFormatRGBA_PVRTC4BPPsrgb
		{GL_ATC_RGB_AMD,								GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBO,	8},			// kTexFormatRGB_ATCunorm
		{GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ATCunorm
		{internalETC1,									GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBO,	16},		// kTexFormatRGB_ETCunorm
		{internalETC1srgb,								GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBO,	16},		// kTexFormatRGB_ETCsrgb
		{GL_COMPRESSED_RGB8_ETC2,						GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBO,	8},			// kTexFormatRGB_ETC2unorm
		{GL_COMPRESSED_SRGB8_ETC2,						GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBO,	8},			// kTexFormatRGB_ETC2srgb
		{GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,	GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBA,	8},			// kTexFormatRGB_A1_ETC2unorm
		{GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,	GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBA,	8},			// kTexFormatRGB_A1_ETC2srgb
		{GL_COMPRESSED_RGBA8_ETC2_EAC,					GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ETC2unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,			GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ETC2srgb
		{GL_COMPRESSED_R11_EAC,							GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRZZO,	8},			// kTexFormatR_EACunorm
		{GL_COMPRESSED_SIGNED_R11_EAC,					GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRZZO,	8},			// kTexFormatR_EACsnorm
		{GL_COMPRESSED_RG11_EAC,						GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGZO,	16},		// kTexFormatRG_EACunorm
		{GL_COMPRESSED_SIGNED_RG11_EAC,					GL_NONE,					GL_NONE,						capETC,			gl::kTextureSwizzleFuncRGZO,	16},		// kTexFormatRG_EACsnorm
		{GL_COMPRESSED_RGBA_ASTC_4x4,					GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC4X4unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC4X4srgb
		{GL_COMPRESSED_RGBA_ASTC_5x5,					GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC5X5unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC5X5srgb
		{GL_COMPRESSED_RGBA_ASTC_6x6,					GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC6X6unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC6X6srgb
		{GL_COMPRESSED_RGBA_ASTC_8x8,					GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC8X8unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC8X8srgb
		{GL_COMPRESSED_RGBA_ASTC_10x10,					GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC10X10unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC10X10srgb
		{GL_COMPRESSED_RGBA_ASTC_12x12,					GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16},		// kTexFormatRGBA_ASTC12X12unorm
		{GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12,			GL_NONE,					GL_NONE,						capCompressed,	gl::kTextureSwizzleFuncRGBA,	16}			// kTexFormatRGBA_ASTC12X12srgb
	};
	CompileTimeAssertArraySize(table, gl::kTexFormatCount);

	std::copy(&table[0], &table[0] + ARRAY_SIZE(table), m_FormatGLES.begin());
}

const FormatDescGLES& TranslateGLES::GetFormatDesc(gl::TexFormat format) const
{
	AssertFormatMsg(format >= gl::kTexFormatFirst && format <= gl::kTexFormatLast, "OPENGL ERROR: TranslateGLES::GetFormatDesc - Invalid input: %d", format);

	return this->m_FormatGLES[format];
}

void TranslateGLES::InitTextureFormat2Format(const GraphicsCaps & caps)
{
	const gl::TexFormat table[] = // kTexFormatTotalCount
	{
		gl::kTexFormatNone,				//kTexFormatNone
		gl::kTexFormatA8unorm,			//kTexFormatAlpha8
		gl::kTexFormatBGRA4unorm,		//kTexFormatARGB4444
		gl::kTexFormatRGB8unorm,		//kTexFormatRGB24
		gl::kTexFormatRGBA8unorm,		//kTexFormatRGBA32
		gl::kTexFormatARGB8unorm,		//kTexFormatARGB32
		gl::kTexFormatRGBA32sf,			//kTexFormatARGBFloat
		gl::kTexFormatR5G6B5unorm,		//kTexFormatRGB565
		gl::kTexFormatBGR8unorm,		//kTexFormatBGR24
		gl::kTexFormatA16unorm,			//kTexFormatAlphaLum16
		gl::kTexFormatRGB_DXT1unorm,	//kTexFormatDXT1
		gl::kTexFormatRGBA_DXT3unorm,	//kTexFormatDXT3
		gl::kTexFormatRGBA_DXT5unorm,	//kTexFormatDXT5
		gl::kTexFormatRGBA4unorm,		//kTexFormatRGBA4444
		gl::kTexFormatBGRA8unorm,		//kTexFormatBGRA32
		gl::kTexFormatR16sf,			//kTexFormatRHalf
		gl::kTexFormatRG16sf,			//kTexFormatRGHalf
		gl::kTexFormatRGBA16sf,			//kTexFormatRGBAHalf
		gl::kTexFormatR32sf,			//kTexFormatRFloat
		gl::kTexFormatRG32sf,			//kTexFormatRGFloat
		gl::kTexFormatRGBA32sf,			//kTexFormatRGBAFloat
		gl::kTexFormatNone,				//kTexFormatYUY2
		gl::kTexFormatNone,				//kTexFormatPCCount
		gl::kTexFormatRGB32sf,			//kTexFormatRGBFloat
		gl::kTexFormatNone, gl::kTexFormatNone, gl::kTexFormatNone, gl::kTexFormatNone,
		gl::kTexFormatNone,				//kTexFormatDXT1Crunched
		gl::kTexFormatNone,				//kTexFormatDXT5Crunched
		gl::kTexFormatRGB_PVRTC2BPPunorm,	//kTexFormatPVRTC_RGB2
		gl::kTexFormatRGBA_PVRTC2BPPunorm,	//kTexFormatPVRTC_RGBA2
		gl::kTexFormatRGB_PVRTC4BPPunorm,	//kTexFormatPVRTC_RGB4
		gl::kTexFormatRGBA_PVRTC4BPPunorm,	//kTexFormatPVRTC_RGBA4
		gl::kTexFormatRGB_ETCunorm,			//kTexFormatETC_RGB4
		gl::kTexFormatRGB_ATCunorm,			//kTexFormatATC_RGB4
		gl::kTexFormatRGBA_ATCunorm,		//kTexFormatATC_RGBA8
		gl::kTexFormatNone,					//kTexReserved12
		gl::kTexFormatNone,					//kTexFormatFlashATF_RGB_DXT1
		gl::kTexFormatNone,					//kTexFormatFlashATF_RGBA_JPG
		gl::kTexFormatNone,					//kTexFormatFlashATF_RGB_JPG
		gl::kTexFormatR_EACunorm,			//kTexFormatEAC_R
		gl::kTexFormatR_EACsnorm,			//kTexFormatEAC_R_SIGNED
		gl::kTexFormatRG_EACunorm,			//kTexFormatEAC_RG
		gl::kTexFormatRG_EACsnorm,			//kTexFormatEAC_RG_SIGNED
		gl::kTexFormatRGB_ETC2unorm,		//kTexFormatETC2_RGB
		gl::kTexFormatRGB_A1_ETC2unorm,		//kTexFormatETC2_RGBA1
		gl::kTexFormatRGBA_ETC2unorm,		//kTexFormatETC2_RGBA8
		gl::kTexFormatRGBA_ASTC4X4unorm,	//kTexFormatASTC_RGB_4x4
		gl::kTexFormatRGBA_ASTC5X5unorm,	//kTexFormatASTC_RGB_5x5
		gl::kTexFormatRGBA_ASTC6X6unorm,	//kTexFormatASTC_RGB_6x6
		gl::kTexFormatRGBA_ASTC8X8unorm,	//kTexFormatASTC_RGB_8x8
		gl::kTexFormatRGBA_ASTC10X10unorm,	//kTexFormatASTC_RGB_10x10
		gl::kTexFormatRGBA_ASTC12X12unorm,	//kTexFormatASTC_RGB_12x12
		gl::kTexFormatRGBA_ASTC4X4unorm,	//kTexFormatASTC_RGBA_4x4
		gl::kTexFormatRGBA_ASTC5X5unorm,	//kTexFormatASTC_RGBA_5x5
		gl::kTexFormatRGBA_ASTC6X6unorm,	//kTexFormatASTC_RGBA_6x6
		gl::kTexFormatRGBA_ASTC8X8unorm,	//kTexFormatASTC_RGBA_8x8
		gl::kTexFormatRGBA_ASTC10X10unorm,	//kTexFormatASTC_RGBA_10x10
		gl::kTexFormatRGBA_ASTC12X12unorm,	//kTexFormatASTC_RGBA_12x12
		gl::kTexFormatNone,					//kTexFormatETC_RGB4_3DS
		gl::kTexFormatNone,					//kTexFormatETC_RGBA8_3DS
	};
	CompileTimeAssertArraySize(table, kTexFormatTotalCount);

	std::copy(&table[0], &table[0] + ARRAY_SIZE(table), this->m_TextureFormat2Format.begin());
}

gl::TexFormat TranslateGLES::GetFormat(TextureFormat format, TextureColorSpace colorSpace) const
{
	AssertFormatMsg(format >= kTexFormatNone && format < kTexFormatTotalCount, "OPENGL ERROR: TranslateGLES::GetTextureFormatGLES - Invalid input: %d", format);

	gl::TexFormat texFormat = this->m_TextureFormat2Format[format];
	if (colorSpace != kTexColorSpaceLinear)
		texFormat = ::MakeSRGB(texFormat);

	return texFormat;
}

const FormatDescGLES& TranslateGLES::GetFormatDesc(TextureFormat format, TextureColorSpace colorSpace) const
{
	return this->GetFormatDesc(this->GetFormat(format, colorSpace));
}

void TranslateGLES::InitRenderTextureFormat2Format(const GraphicsCaps & caps)
{
	const gl::TexFormat table[] = // kRTFormatCount
	{
		gl::kTexFormatRGBA8unorm,										// kRTFormatARGB32
		UNITY_WEBGL ? gl::kTexFormatRGBA4unorm : gl::kTexFormatNone,	// kRTFormatDepth
		gl::kTexFormatRGBA16sf,											// kRTFormatARGBHalf
		UNITY_WEBGL ? gl::kTexFormatRGBA4unorm : gl::kTexFormatNone,	// kRTFormatShadowMap
		gl::kTexFormatR5G6B5unorm,										// kRTFormatRGB565
		gl::kTexFormatRGBA4unorm,										// kRTFormatARGB4444
		gl::kTexFormatRGB5A1unorm,										// kRTFormatARGB1555
		UNITY_WEBGL ? gl::kTexFormatRGBA4unorm : gl::kTexFormatNone,	// kRTFormatDefault
		gl::kTexFormatRGB10A2unorm,										// kRTFormatA2R10G10B10
		UNITY_WEBGL ? gl::kTexFormatRGBA4unorm : gl::kTexFormatNone,	// kRTFormatDefaultHDR
		UNITY_WEBGL ? gl::kTexFormatRGBA4unorm : gl::kTexFormatNone,	// kRTFormatARGB64
		gl::kTexFormatRGBA32sf,											// kRTFormatARGBFloat
		gl::kTexFormatRG32sf,											// kRTFormatRGFloat
		gl::kTexFormatRG16sf,											// kRTFormatRGHalf
		gl::kTexFormatR32sf,											// kRTFormatRFloat
		gl::kTexFormatR16sf,											// kRTFormatRHalf
		gl::kTexFormatR8unorm,											// kRTFormatR8
		gl::kTexFormatRGBA32i,											// kRTFormatARGBInt
		gl::kTexFormatRG32i,											// kRTFormatRGInt
		gl::kTexFormatR32i,												// kRTFormatRInt
		gl::kTexFormatBGRA8unorm,										// kRTFormatBGRA32
		static_cast<gl::TexFormat>(gl::kTexFormatInvalid),				// kRTFormatVideo
		gl::kTexFormatRG11B10uf,										// kRTFormatR11G11B10Float
	};
	CompileTimeAssertArraySize(table, kRTFormatCount);

	std::copy(&table[0], &table[0] + ARRAY_SIZE(table), this->m_RenderTextureFormat2Format.begin());
}

gl::TexFormat TranslateGLES::GetFormat(RenderTextureFormat format, TextureColorSpace colorSpace) const
{
	AssertFormatMsg(format >= kRTFormatFirst && format < kRTFormatCount, "OPENGL ERROR: TranslateGLES::GetFormatDesc - Invalid input: %d", format);

	gl::TexFormat texFormat = this->m_RenderTextureFormat2Format[format];
	if (colorSpace != kTexColorSpaceLinear)
		texFormat = MakeSRGB(texFormat);

	return texFormat;
}

const FormatDescGLES& TranslateGLES::GetFormatDesc(RenderTextureFormat format, TextureColorSpace colorSpace) const
{
	AssertFormatMsg(format >= kRTFormatFirst && format < kRTFormatCount, "OPENGL ERROR: TranslateGLES::GetFormatDesc - Invalid input: %d", format);

	return this->GetFormatDesc(this->GetFormat(format, colorSpace));
}

void TranslateGLES::InitDepthBufferFormat2Format(const GraphicsCaps & caps)
{
	const gl::TexFormat table[] = // kDepthFormatCount
	{
		gl::kTexFormatNone,			// kDepthFormatNone
		gl::kTexFormatDepth16,		// kDepthFormatMin16bits_NoStencil
		gl::kTexFormatDepth24,		// kDepthFormatMin24bits_Stencil
	};
	CompileTimeAssertArraySize(table, kDepthFormatCount);

	std::copy(&table[0], &table[0] + ARRAY_SIZE(table), this->m_DepthBufferFormat2Format.begin());
}

gl::TexFormat TranslateGLES::GetFormat(DepthBufferFormat format) const
{
	AssertFormatMsg(format >= kDepthFormatNone && format < kDepthFormatCount, "OPENGL ERROR: TranslateGLES::GetFormatDesc - Invalid input: %d", format);
	return this->m_DepthBufferFormat2Format[format];
}

const FormatDescGLES& TranslateGLES::GetFormatDesc(DepthBufferFormat format) const
{
	return this->GetFormatDesc(this->GetFormat(format));
}

// -- gl::BufferTarget to OpenGL enum translation --

void TranslateGLES::InitBufferTarget(const GraphicsCaps & caps, GfxDeviceLevelGL level)
{
	const GLenum translation[] =
	{
		GL_ELEMENT_ARRAY_BUFFER,									// kElementArrayBuffer
		GL_ARRAY_BUFFER,											// kArrayBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_COPY_WRITE_BUFFER,			// kCopyWriteBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_COPY_READ_BUFFER,			// kCopyReadBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_PIXEL_PACK_BUFFER,			// kPixelPackBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_PIXEL_UNPACK_BUFFER,		// kPixelUnpackBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_DISPATCH_INDIRECT_BUFFER,	// kDispatchIndirectBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_DRAW_INDIRECT_BUFFER,		// kDrawIndirectBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_PARAMETER_BUFFER_ARB,		// kParameterBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_QUERY_BUFFER,				// kQueryBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_UNIFORM_BUFFER,				// kUniformBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_TRANSFORM_FEEDBACK_BUFFER,	// kTransformFeedbackBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_SHADER_STORAGE_BUFFER,		// kShaderStorageBuffer
		UNITY_WEBGL ? kInvalidEnum : GL_ATOMIC_COUNTER_BUFFER		// kAtomicCounterBuffer
	};
	CompileTimeAssertArraySize(translation, gl::kBufferTargetCount);

	std::copy(&translation[0], &translation[0] + ARRAY_SIZE(translation), m_BufferTarget.begin());
}

GLenum TranslateGLES::GetBufferTarget(gl::BufferTarget target) const
{
	AssertFormatMsg(target >= gl::kBufferTargetFirst && target <= gl::kBufferTargetLast, "OPENGL ERROR: TranslateGLES::BufferTarget - Invalid input: %d", target);

	const GLenum Translation = m_BufferTarget[target];
	DebugAssertMsg(Translation != kInvalidEnum, "OPENGL ERROR: TranslateGLES::BufferTarget - Invalid output");

	return Translation;
}

// -- CompareFunction to OpenGL enum translation --

GLenum TranslateGLES::Func(CompareFunction func) const
{
	static const GLenum translation[] =
	{
		GL_ALWAYS,		// kFuncDisabled
		GL_NEVER,		// kFuncNever
		GL_LESS,		// kFuncLess
		GL_EQUAL,		// kFuncEqual
		GL_LEQUAL,		// kFuncLEqual
		GL_GREATER,		// kFuncGreater
		GL_NOTEQUAL,	// kFuncNotEqual
		GL_GEQUAL,		// kFuncGEqual
		GL_ALWAYS		// kFuncAlways
	};
	CompileTimeAssertArraySize(translation, kFuncCount);

	DebugAssertMsg(func != kFuncUnknown, "OPENGL ERROR: Invalid CompareFunction input value");
	const GLenum translated = translation[func];

	DebugAssertMsg(translated != kInvalidEnum, "OPENGL ERROR: Invalid 'CompareFunction' output value");
	return translated;
}

// -- StencilOp to OpenGL enum translation --

GLenum TranslateGLES::StencilOperation(StencilOp op) const
{
	static const GLenum translation[] =
	{
		GL_KEEP,		// kStencilOpKeep
		GL_ZERO,		// kStencilOpZero
		GL_REPLACE,		// kStencilOpReplace
		GL_INCR,		// kStencilOpIncrSat
		GL_DECR,		// kStencilOpDecrSat
		GL_INVERT,		// kStencilOpInvert
		GL_INCR_WRAP,	// kStencilOpIncrWrap
		GL_DECR_WRAP	// kStencilOpDecrWrap
	};
	CompileTimeAssertArraySize(translation, kStencilOpCount);

	const GLenum translatd = translation[op];
	DebugAssertMsg(translatd != kInvalidEnum, "OPENGL ERROR: Invalid 'StencilOp' output value");

	return translatd;
}

// -- BlendMode to OpenGL enum translation --

GLenum TranslateGLES::BlendFactor(BlendMode mode) const
{
	static const GLenum translation[] =
	{
		GL_ZERO,					// kBlendZero
		GL_ONE,						// kBlendOne
		GL_DST_COLOR,				// kBlendDstColor
		GL_SRC_COLOR,				// kBlendSrcColor
		GL_ONE_MINUS_DST_COLOR,		// kBlendOneMinusDstColor
		GL_SRC_ALPHA,				// kBlendSrcAlpha
		GL_ONE_MINUS_SRC_COLOR,		// kBlendOneMinusSrcColor
		GL_DST_ALPHA,				// kBlendDstAlpha
		GL_ONE_MINUS_DST_ALPHA,		// kBlendOneMinusDstAlpha
		GL_SRC_ALPHA_SATURATE,		// kBlendSrcAlphaSaturate
		GL_ONE_MINUS_SRC_ALPHA		// kBlendOneMinusSrcAlpha
	};
	CompileTimeAssertArraySize(translation, kBlendCount);

	DebugAssertMsg(mode >= kBlendFirst && mode < kBlendCount, "OPENGL ERROR: Invalid 'CompareFunction' input value");
	const GLenum translated = translation[mode];

	DebugAssertMsg(translated != kInvalidEnum, "OPENGL ERROR: Invalid 'CompareFunction' output value");
	return translated;
}

GLenum TranslateGLES::BlendEquation(BlendOp equation) const
{
	static const GLenum translation[] =
	{
		GL_FUNC_ADD,					// kBlendOpAdd
		GL_FUNC_SUBTRACT,				// kBlendOpSub
		GL_FUNC_REVERSE_SUBTRACT,		// kBlendOpRevSub
		GL_MIN,							// kBlendOpMin, GL 1.1, ES 3.0,  GL_EXT_blend_minmax
		GL_MAX,							// kBlendOpMax, GL 1.1, ES 3.0, GL_EXT_blend_minmax

		// Logical operation are not supported
		kInvalidEnum, kInvalidEnum, kInvalidEnum, kInvalidEnum,
		kInvalidEnum, kInvalidEnum, kInvalidEnum, kInvalidEnum,
		kInvalidEnum, kInvalidEnum, kInvalidEnum, kInvalidEnum,
		kInvalidEnum, kInvalidEnum, kInvalidEnum, kInvalidEnum,
	
		GL_MULTIPLY_KHR,			// kBlendOpMultiply
		GL_SCREEN_KHR,				// kBlendOpScreen
		GL_OVERLAY_KHR,				// kBlendOpOverlay
		GL_DARKEN_KHR,				// kBlendOpDarken
		GL_LIGHTEN_KHR,				// kBlendOpLighten
		GL_COLORDODGE_KHR,			// kBlendOpColorDodge
		GL_COLORBURN_KHR,			// kBlendOpColorBurn
		GL_HARDLIGHT_KHR,			// kBlendOpHardLight
		GL_SOFTLIGHT_KHR,			// kBlendOpSoftLight
		GL_DIFFERENCE_KHR,			// kBlendOpDifference
		GL_EXCLUSION_KHR,			// kBlendOpExclusion
		GL_HSL_HUE_KHR,				// kBlendOpHSLHue
		GL_HSL_SATURATION_KHR,		// kBlendOpHSLSaturation
		GL_HSL_COLOR_KHR,			// kBlendOpHSLColor
		GL_HSL_LUMINOSITY_KHR		// kBlendOpHSLLuminosity
	};
	CompileTimeAssertArraySize(translation, kBlendOpCount);

	DebugAssertMsg(equation >= kBlendOpFirst && equation < kBlendOpCount, "OPENGL ERROR: Invalid 'BlendOp' input value");
	const GLenum translated = translation[equation];

	DebugAssertMsg(translated != kInvalidEnum, "OPENGL ERROR: Invalid 'BlendOp' output value");
	return translated;
}

// -- VertexChannelFormat to OpenGL enum translation --

void TranslateGLES::InitVertexType(const GraphicsCaps & caps, GfxDeviceLevelGL level)
{
	const GLenum translation[] =
	{
		GL_FLOAT,													// kChannelFormatFloat
		IsGfxLevelES2(level) ? GL_HALF_FLOAT_OES : GL_HALF_FLOAT,	// kChannelFormatFloat16
		GL_UNSIGNED_BYTE,											// kChannelFormatColor
		GL_BYTE,													// kChannelFormatByte
		GL_UNSIGNED_INT												// kChannelFormatUInt32
	};
	CompileTimeAssertArraySize(translation, kChannelFormatCount);

	std::copy(&translation[0], &translation[0] + ARRAY_SIZE(translation), m_VertexChannelFormat.begin());
}

GLenum TranslateGLES::VertexType(VertexChannelFormat format) const
{
	DebugAssertMsg(format >= kChannelFormatFirst && format <= kChannelFormatCount, "OPENGL ERROR: TranslateGLES::VertexType - Invalid input");
	const GLenum translation = m_VertexChannelFormat[format];
	
	DebugAssertMsg(translation != kInvalidEnum, "OPENGL ERROR: TranslateGLES::VertexType - Invalid output");
	return translation;
}

// -- gl::ObjectType to OpenGL enum translation --

void TranslateGLES::InitObjectType(const GraphicsCaps & caps)
{
	const GLenum translation[] =
	{
		caps.gles.hasDebugKHR ? GL_BUFFER : GL_BUFFER_OBJECT_EXT,						// kBuffer
		caps.gles.hasDebugKHR ? GL_SHADER : GL_SHADER_OBJECT_EXT,						// kShader
		caps.gles.hasDebugKHR ? GL_PROGRAM : GL_PROGRAM_OBJECT_EXT,						// kProgram
		caps.gles.hasDebugKHR ? GL_VERTEX_ARRAY : GL_VERTEX_ARRAY_OBJECT_EXT,			// kVertexArray
		caps.gles.hasDebugKHR ? GL_QUERY : GL_QUERY_OBJECT_EXT,							// kQuery
		caps.gles.hasDebugKHR ? GL_PROGRAM_PIPELINE : GL_PROGRAM_PIPELINE_OBJECT_EXT,	// kProgramPipeline
		GL_TRANSFORM_FEEDBACK,															// kTransformFeedback
		GL_SAMPLER,																		// kSampler
		GL_TEXTURE,																		// kTexture
		GL_RENDERBUFFER,																// kRenderbuffer
		GL_FRAMEBUFFER																	// kFramebuffer
	};
	CompileTimeAssertArraySize(translation, gl::kObjectTypeCount);

	std::copy(&translation[0], &translation[0] + ARRAY_SIZE(translation), m_ObjectType.begin());
}

GLenum TranslateGLES::ObjectType(gl::ObjectType type) const
{
	DebugAssertMsg(type >= gl::kObjectTypeFirst && type <= gl::kObjectTypeLast, "OPENGL ERROR: Unsupported gl::ObjectType input format translation");

	const GLenum translation = m_ObjectType[type];
	DebugAssertMsg(translation != kInvalidEnum, "OPENGL ERROR: Unsupported gl::ObjectType output format translation");

	return translation;
}

// -- gl::ShaderStage to OpenGL enum translation --

static const GLenum kShaderStageTranslation[] = // gl::kShaderStageCount
{
	GL_VERTEX_SHADER,						// gl::kVertexShaderStage
	GL_TESS_CONTROL_SHADER,					// gl::kControlShaderStage
	GL_TESS_EVALUATION_SHADER,				// gl::kEvalShaderStage
	GL_GEOMETRY_SHADER,						// gl::kGeometryShaderStage
	GL_FRAGMENT_SHADER,						// gl::kFragmentShaderStage
	GL_COMPUTE_SHADER,						// gl::kComputeShaderStage
};
CompileTimeAssertArraySize(kShaderStageTranslation, gl::kShaderStageCount);

GLenum TranslateGLES::GetShaderStage(gl::ShaderStage stage) const
{
	const GLenum translated = kShaderStageTranslation[stage];
	Assert(translated != kInvalidEnum);

	return translated;
}

gl::ShaderStage TranslateGLES::GetShaderStage(GLenum stage) const
{
	for (int stageStageIndex = 0; stageStageIndex < gl::kShaderStageCount; ++stageStageIndex)
		if (kShaderStageTranslation[stageStageIndex] == stage)
			return static_cast<gl::ShaderStage>(stageStageIndex);

	DebugAssertMsg(0, "OPENGL ERROR: TranslateGLES::GetShaderStage - Invalid input enum");
	return static_cast<gl::ShaderStage>(kInvalidEnum);
}

const char* TranslateGLES::GetShaderTitle(gl::ShaderStage stage) const
{
	static const char* translation[] =				// gl::kShaderStageCount
	{
		"vertex shader",							// gl::kVertexShaderStage
		"tessellation control shader",				// gl::kControlShaderStage
		"tessellation evaluation shader",			// gl::kEvalShaderStage
		"geometry evaluation shader",				// gl::kGeometryShaderStage
		"fragment evaluation shader",				// gl::kFragmentShaderStage
		"compute evaluation shader"					// gl::kComputeShaderStage
	};
	CompileTimeAssertArraySize(translation, gl::kShaderStageCount);

	Assert(stage >= gl::kShaderStageFirst && stage <= gl::kShaderStageLast);
	return translation[stage];
}

// -- GfxPrimitiveType to OpenGL enum translation --

GLenum TranslateGLES::Topology(GfxPrimitiveType type) const
{
	static const GLenum translation[] =
	{
		GL_TRIANGLES,			// kPrimitiveTriangles
		GL_TRIANGLE_STRIP,		// kPrimitiveTriangleStrip
		kInvalidEnum,			// kPrimitiveQuads
		GL_LINES,				// kPrimitiveLines
		GL_LINE_STRIP,			// kPrimitiveLineStrip
		GL_POINTS				// kPrimitivePoints
	};
	CompileTimeAssertArraySize(translation, kPrimitiveTypeCount);

	DebugAssertMsg(type >= kPrimitiveTypeFirst && type <= kPrimitiveTypeLast, "OPENGL ERROR: Invalid input primitive type");

	const GLenum translated = translation[type];
	Assert(translated != kInvalidEnum);

	return translated;
}

// -- gl::FramebufferTarget to OpenGL enum translation --

void TranslateGLES::InitFramebufferTarget(const GraphicsCaps & caps)
{
	const GLenum translation[] = // gl::kFramebufferTargetCount
	{
		caps.gles.hasReadDrawFramebuffer ? GL_DRAW_FRAMEBUFFER : GL_FRAMEBUFFER,	// kDrawFramebuffer
		caps.gles.hasReadDrawFramebuffer ? GL_READ_FRAMEBUFFER : GL_FRAMEBUFFER		// kReadFramebuffer
	};
	CompileTimeAssertArraySize(translation, gl::kFramebufferTargetCount);

	std::copy(&translation[0], &translation[0] + ARRAY_SIZE(translation), m_FramebufferTarget.begin());
}

GLenum TranslateGLES::FramebufferTarget(gl::FramebufferTarget target) const
{
	Assert(target >= gl::kFramebufferTargetFirst && target <= gl::kFramebufferTargetLast);

	const GLenum translated = m_FramebufferTarget[target];
	Assert(translated != kInvalidEnum);

	return translated;
}

GLenum TranslateGLES::FramebufferRead(gl::FramebufferRead read) const
{
	Assert(read >= gl::kFramebufferReadFirst && read <= gl::kFramebufferReadLast && read != gl::kFramebufferReadDefault);

	static const GLenum translation[] = // gl::kFramebufferTargetCount
	{
		GL_NONE,					// kFramebufferReadNone
		kInvalidEnum,				// kFramebufferReadDefault
		GL_BACK,					// kFramebufferReadBack
		GL_COLOR_ATTACHMENT0 + 0,	// kFramebufferReadColor0
		GL_COLOR_ATTACHMENT0 + 1,	// kFramebufferReadColor1
		GL_COLOR_ATTACHMENT0 + 2,	// kFramebufferReadColor2
		GL_COLOR_ATTACHMENT0 + 3,	// kFramebufferReadColor3
		GL_COLOR_ATTACHMENT0 + 4,	// kFramebufferReadColor4
		GL_COLOR_ATTACHMENT0 + 5,	// kFramebufferReadColor5
		GL_COLOR_ATTACHMENT0 + 6,	// kFramebufferReadColor6
		GL_COLOR_ATTACHMENT0 + 7	// kFramebufferReadColor7
	};
	CompileTimeAssertArraySize(translation, gl::kFramebufferReadCount);

	const GLenum translated = translation[read];
	Assert(translated != kInvalidEnum);

	return translated;
}

// -- TextureFilterMode to OpenGL enum translation --

GLenum TranslateGLES::FilterMin(TextureFilterMode filter, bool hasMipMaps) const
{
	static const GLenum translationTable[] =
	{
		GL_NEAREST,						// kTexFilterNearest
		GL_LINEAR,						// kTexFilterBilinear
		kInvalidEnum					// kTexFilterTrilinear, needs mipmaps
	};
	CompileTimeAssertArraySize(translationTable, kTexFilterCount);

	static const GLenum translationTableMipMaps[] =
	{
		GL_NEAREST_MIPMAP_NEAREST,		// kTexFilterNearest
		GL_LINEAR_MIPMAP_NEAREST,		// kTexFilterBilinear
		GL_LINEAR_MIPMAP_LINEAR			// kTexFilterTrilinear
	};
	CompileTimeAssertArraySize(translationTableMipMaps, kTexFilterCount);

	GLenum translated = hasMipMaps ? translationTableMipMaps[filter] : translationTable[filter];
	Assert(translated != kInvalidEnum);
	return translated;
}

// -- TextureFilterMode to OpenGL enum translation --

GLenum TranslateGLES::FilterMag(TextureFilterMode filter) const
{
	static const GLenum translation[] =
	{
		GL_NEAREST,						// kTexFilterNearest
		GL_LINEAR,						// kTexFilterBilinear
		GL_LINEAR						// kTexFilterTrilinear
	};
	CompileTimeAssertArraySize(translation, kTexFilterCount);

	const GLenum translated = translation[filter];
	Assert(translated != kInvalidEnum);

	return translated;
}

// -- TextureWrapMode to OpenGL enum translation --

GLenum TranslateGLES::Wrap(TextureWrapMode wrap) const
{
	static const GLenum translation[] =
	{
		GL_REPEAT,					// kTexWrapRepeat
		GL_CLAMP_TO_EDGE			// kTexWrapClamp
		//GL_MIRRORED_REPEAT			// N/A
	};
	CompileTimeAssertArraySize(translation, kTexWrapCount);

	const GLenum translated = translation[wrap];
	Assert(translated != kInvalidEnum);
	return translated;
}

// -- gl::FramebufferMask to OpenGL enum translation --

GLenum TranslateGLES::FramebufferMask(gl::FramebufferMask Mask) const
{
	static const GLenum translation[] =
	{
		GL_COLOR_BUFFER_BIT,						// kFramebufferColorBit
		GL_DEPTH_BUFFER_BIT,						// kFramebufferDepthBit
		GL_STENCIL_BUFFER_BIT,						// kFramebufferStencilBit
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,	// kFramebufferColorDepthBit
	};
	CompileTimeAssertArraySize(translation, gl::kFramebufferMaskCount);

	const GLenum translated = translation[Mask];
	Assert(translated != kInvalidEnum);
	return translated;
}

// -- gl::EnabledCap to OpenGL enum translation --

GLenum TranslateGLES::Enable(gl::EnabledCap cap) const
{
	static const GLenum translation[] =
	{
		GL_BLEND,							//kBlend,
		GL_COLOR_LOGIC_OP,					//kColorLogicOp,
		GL_CULL_FACE,						//kCullFace,
		GL_DEBUG_OUTPUT,					//kDebugOutput
		GL_DEBUG_OUTPUT_SYNCHRONOUS,		//kDebugOutputSynchronous
		GL_DEPTH_CLAMP,						//kDepthClamp,
		GL_DEPTH_TEST,						//kDepthTest,
		GL_DITHER,							//kDither,
		GL_FRAMEBUFFER_SRGB,				//kFramebufferSRGB,
		GL_LINE_SMOOTH,						//kLineSmooth,
		GL_MULTISAMPLE,						//kMultisample,
		GL_POLYGON_OFFSET_FILL,				//kPolygonOffsetFill,
		GL_POLYGON_OFFSET_LINE,				//kPolygonOffsetLine,
		GL_POLYGON_OFFSET_POINT,			//kPolygonOffsetpoint,
		GL_POLYGON_SMOOTH,					//kPolygonSmooth,
		GL_PRIMITIVE_RESTART,				//kPrimitiveRestart,
		GL_PRIMITIVE_RESTART_FIXED_INDEX,	//kPrimitiveRestartFixedIndex,
		GL_RASTERIZER_DISCARD,				//kRasterizerDiscard,
		GL_SAMPLE_ALPHA_TO_COVERAGE,		//kSampleAlphaToCoverage,
		GL_SAMPLE_ALPHA_TO_ONE,				//kSampleAlphaToOne,
		GL_SAMPLE_COVERAGE,					//kSampleCoverage,
		GL_SAMPLE_SHADING,					//kSampleShading,
		GL_SAMPLE_MASK,						//kSampleMask,
		GL_SCISSOR_TEST,					//kScissorTest,
		GL_STENCIL_TEST,					//kStencilTest,
		GL_TEXTURE_CUBE_MAP_SEAMLESS,		//kTextureCubeMapSeamless,
		GL_PROGRAM_POINT_SIZE,				//kProgramPointSize,
	};
	CompileTimeAssertArraySize(translation, gl::kEnabledCapCount);

	const GLenum translated = translation[cap];
	Assert(translated != kInvalidEnum);
	return translated;
}

// -- VertexArray caching translation --

unsigned int TranslateGLES::VertexArrayKindBitfield(gl::VertexArrayAttribKind kind) const
{
	static const unsigned int translation[] =
	{
		gl::kPointerBits,				// kPointer
		gl::kPointerNormalizedBits,		// kPointerNormalized
		gl::kPointerIBits,				// kIPointer
		gl::kPointerLBits				// kLPointer
	};
	CompileTimeAssertArraySize(translation, gl::kVertexArrayAttribCount);

	return translation[kind];
}

unsigned int TranslateGLES::VertexArraySizeBitfield(GLint size) const
{
	static const unsigned int Translation[] =
	{
		gl::kVec1Bits,		// 1
		gl::kVec2Bits,		// 2
		gl::kVec3Bits,		// 3
		gl::kVec4Bits		// 4
	};

	Assert(size > 0 && size <= 4);

	return Translation[size - 1];
}

unsigned int TranslateGLES::VertexArrayTypeBitfield(VertexChannelFormat format) const
{
	static const unsigned int translation[] =
	{
		gl::kF32Bits,		// kChannelFormatFloat
		gl::kF16Bits,		// kChannelFormatFloat16
		gl::kU8Bits,		// kChannelFormatColor
		gl::kI8Bits,		// kChannelFormatByte
		gl::kU32Bits		// kChannelFormatUInt32
	};
	CompileTimeAssertArraySize(translation, kChannelFormatCount);

	Assert(format < kChannelFormatCount);
	return translation[format];
}

// -- MemoryBarrierType to OpenGL memory barrier bits translation --

GLenum TranslateGLES::MemoryBarrierBits(gl::MemoryBarrierType type) const
{
	static const GLenum translation[] =
	{
		GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT,	//kBarrierVertexAttribArray
		GL_ELEMENT_ARRAY_BARRIER_BIT,		//kBarrierElementArray
		GL_UNIFORM_BARRIER_BIT,				//kBarrierUniform
		GL_TEXTURE_FETCH_BARRIER_BIT,		//kBarrierTextureFetch
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, //kBarrierShaderImageAccess
		GL_COMMAND_BARRIER_BIT,				//kBarrierCommand
		GL_PIXEL_BUFFER_BARRIER_BIT,		//kBarrierPixelBuffer
		GL_TEXTURE_UPDATE_BARRIER_BIT,		//kBarrierTextureUpdate
		GL_BUFFER_UPDATE_BARRIER_BIT,		//kBarrierBufferUpdate
		GL_FRAMEBUFFER_BARRIER_BIT,			//kBarrierFramebuffer
		GL_TRANSFORM_FEEDBACK_BARRIER_BIT,	//kBarrierTransformFeedback
		GL_ATOMIC_COUNTER_BARRIER_BIT,		//kBarrierAtomicCounter
		GL_SHADER_STORAGE_BARRIER_BIT		//kBarrierShaderStorage
	};
	CompileTimeAssertArraySize(translation, gl::kBarrierTypeCount);

	return translation[type];
}
