#pragma once

#define VERTEX_FORMAT1(a) (1 << kShaderChannel##a)
#define VERTEX_FORMAT2(a, b) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b))
#define VERTEX_FORMAT3(a, b, c) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b) | (1 << kShaderChannel##c))
#define VERTEX_FORMAT5(a,b,c,d,e) ((1 << kShaderChannel##a) | (1 << kShaderChannel##b) | (1 << kShaderChannel##c) | (1 << kShaderChannel##d) | (1 << kShaderChannel##e))

enum GfxBufferTarget
{
	kGfxBufferTargetVertex, kGfxBufferTargetFirst = kGfxBufferTargetVertex,
	kGfxBufferTargetIndex, kGfxBufferTargetLast = kGfxBufferTargetIndex,
};

enum GfxBufferMode
{
	kGfxBufferModeImmutable,	// Static geometry, can't get pointers to write into the buffer
	kGfxBufferModeDynamic,		// Dynamic geometry, D3D9 buffers will be lost on device loss
	kGfxBufferModeCircular,		// Circular writing to and drawing from buffer, caller must not
	// overwrite data the GPU is using except when writing at pos 0!
	kGfxBufferModeStreamOut,	// Destination buffer for stream out

	kGfxBufferModeDynamicForceSystemOnOSX,

	kGfxBufferModeCount // keep this last!
};

enum GfxBufferLabel
{
	kGfxBufferLabelDefault,
	kGfxBufferLabelInternal,
};

enum
{
	kGfxBufferTargetCount = kGfxBufferTargetLast - kGfxBufferTargetFirst + 1
};

enum CompareFunction
{
	kFuncUnknown = -1,
	kFuncDisabled = 0,
	kFuncNever,
	kFuncLess,
	kFuncEqual,
	kFuncLEqual,
	kFuncGreater,
	kFuncNotEqual,
	kFuncGEqual,
	kFuncAlways,
	kFuncCount
};

enum CullMode
{
	kCullUnknown = -1,
	kCullOff = 0,
	kCullFront,
	kCullBack,
	kCullCount
};

enum ColorWriteMask
{
	kColorWriteA = 1,
	kColorWriteB = 2,
	kColorWriteG = 4,
	kColorWriteR = 8,
	kColorWriteAll = (kColorWriteR | kColorWriteG | kColorWriteB | kColorWriteA)
};

enum BlendOp
{
	kBlendOpFirst = 0,
	kBlendOpAdd = kBlendOpFirst,
	kBlendOpSub,
	kBlendOpRevSub,
	kBlendOpMin,
	kBlendOpMax,
	kBlendOpLogicalClear,
	kBlendOpLogicalSet,
	kBlendOpLogicalCopy,
	kBlendOpLogicalCopyInverted,
	kBlendOpLogicalNoop,
	kBlendOpLogicalInvert,
	kBlendOpLogicalAnd,
	kBlendOpLogicalNand,
	kBlendOpLogicalOr,
	kBlendOpLogicalNor,
	kBlendOpLogicalXor,
	kBlendOpLogicalEquiv,
	kBlendOpLogicalAndReverse,
	kBlendOpLogicalAndInverted,
	kBlendOpLogicalOrReverse,
	kBlendOpLogicalOrInverted,
	kBlendOpMultiply,
	kBlendOpScreen,
	kBlendOpOverlay,
	kBlendOpDarken,
	kBlendOpLighten,
	kBlendOpColorDodge,
	kBlendOpColorBurn,
	kBlendOpHardLight,
	kBlendOpSoftLight,
	kBlendOpDifference,
	kBlendOpExclusion,
	kBlendOpHSLHue,
	kBlendOpHSLSaturation,
	kBlendOpHSLColor,
	kBlendOpHSLLuminosity,
	kBlendOpCount,
};

enum BlendMode
{
	kBlendFirst = 0,
	kBlendZero = kBlendFirst,
	kBlendOne,
	kBlendDstColor,
	kBlendSrcColor,
	kBlendOneMinusDstColor,
	kBlendSrcAlpha,
	kBlendOneMinusSrcColor,
	kBlendDstAlpha,
	kBlendOneMinusDstAlpha,
	kBlendSrcAlphaSaturate,
	kBlendOneMinusSrcAlpha,
	kBlendCount
};

enum StencilOp
{
	kStencilOpKeep = 0,
	kStencilOpZero,
	kStencilOpReplace,
	kStencilOpIncrSat,
	kStencilOpDecrSat,
	kStencilOpInvert,
	kStencilOpIncrWrap,
	kStencilOpDecrWrap,
	kStencilOpCount
};

enum
{
	kMaxSupportedTextureUnits = 32,
	kMaxSupportedVertexLights = 8,
	kMaxSupportedTextureCoords = 8,

	kMaxSupportedRenderTargets = 8,
	kMaxSupportedConstantBuffers = 16,
	kMaxSupportedComputeResources = 16,
};

// Ordering is like this so it matches valid D3D9 FVF layouts
// VertexDataLayout in script uses same order and must be kept in sync
// Range must fit into an SInt8
enum ShaderChannel
{
	kShaderChannelNone = -1,
	kShaderChannelVertex = 0,	// Vertex (vector3)
	kShaderChannelNormal,		// Normal (vector3)
	kShaderChannelColor,		// Vertex color
	kShaderChannelTexCoord0,	// Texcoord 0
	kShaderChannelTexCoord1,	// Texcoord 1
	kShaderChannelTexCoord2,	// Texcoord 2
	kShaderChannelTexCoord3,	// Texcoord 3
#if GFX_HAS_TWO_EXTRA_TEXCOORDS
	kShaderChannelTexCoord4,	// Texcoord 4
	kShaderChannelTexCoord5,	// Texcoord 5
#endif
	kShaderChannelTangent,		// Tangent (vector4)
	kShaderChannelCount,			// Keep this last!
};

enum
{
#if !GFX_HAS_TWO_EXTRA_TEXCOORDS
	kMaxTexCoordShaderChannels = 4,
#else
	kMaxTexCoordShaderChannels = 6,
#endif
	kTexCoordShaderChannelsMask = ((1 << kMaxTexCoordShaderChannels) - 1) << kShaderChannelTexCoord0
};

enum ShaderChannelMask
{
	kShaderChannelsHot = (1 << kShaderChannelVertex) | (1 << kShaderChannelNormal) | (1 << kShaderChannelTangent),
	kShaderChannelsCold = (1 << kShaderChannelColor) | kTexCoordShaderChannelsMask,
	kShaderChannelsAll = kShaderChannelsHot | kShaderChannelsCold
};


enum VertexComponent
{
	kVertexCompNone = -1,
	kVertexCompVertex,
	kVertexCompColor,
	kVertexCompNormal,
	kVertexCompTexCoord,
	kVertexCompTexCoord0, kVertexCompTexCoord1, kVertexCompTexCoord2, kVertexCompTexCoord3,
	kVertexCompTexCoord4, kVertexCompTexCoord5, kVertexCompTexCoord6, kVertexCompTexCoord7,
	kVertexCompAttrib0, kVertexCompAttrib1, kVertexCompAttrib2, kVertexCompAttrib3,
	kVertexCompAttrib4, kVertexCompAttrib5, kVertexCompAttrib6, kVertexCompAttrib7,
	kVertexCompAttrib8, kVertexCompAttrib9, kVertexCompAttrib10, kVertexCompAttrib11,
	kVertexCompAttrib12, kVertexCompAttrib13, kVertexCompAttrib14, kVertexCompAttrib15,
	kVertexCompCount // keep this last!
};

enum GfxPrimitiveType
{
	kPrimitiveInvalid = -1,

	kPrimitiveTriangles = 0, kPrimitiveTypeFirst = kPrimitiveTriangles,
	kPrimitiveTriangleStrip,
	kPrimitiveQuads,
	kPrimitiveLines,
	kPrimitiveLineStrip,
	kPrimitivePoints, kPrimitiveTypeLast = kPrimitivePoints,

	kPrimitiveForce32BitInt = 0x7fffffff // force 32 bit enum size
};

// Graphics device identifiers in Unity
enum GfxDeviceRenderer
{
    kGfxRendererOpenGL = 0,          // OpenGL
    kGfxRendererD3D9,                // Direct3D 9
    kGfxRendererD3D11,               // Direct3D 11
    kGfxRendererGCM,                 // Sony PlayStation 3 GCM
    kGfxRendererNull,                // "null" device (used in batch mode)
    kGfxRendererHollywood,           // Nintendo Wii
    kGfxRendererXenon,               // Xbox 360
    kGfxRendererOpenGLES,            // OpenGL ES 1.1
    kGfxRendererOpenGLES20Mobile,    // OpenGL ES 2.0 mobile variant
    kGfxRendererMolehill,            // Flash 11 Stage3D
    kGfxRendererOpenGLES20Desktop,   // OpenGL ES 2.0 desktop variant (i.e. NaCl)
    kGfxRendererOpenGLES30 = 11,
    kGfxRendererMetal = 16,
    kGfxRendererOpenGLCore = 17, // OpenGL 3.x/4.x
    kGfxRendererCount
};

#if GFX_SUPPORTS_OPENGL_UNIFIED

// These enums have 2 usages: for context creation telling what kind of context and which version it should be,
// and for emulation levels to clamp features to match certain GL (/ES) version
// For normal (non-emulated) usage both values can be set to kGfxLevelMax(ES/Desktop)
enum GfxDeviceLevelGL
{
    kGfxLevelUninitialized = 0, // Initial value, should never be used. Marks that caps have not been initialized yet.
    kGfxLevelES2, kGfxLevelFirst = kGfxLevelES2, kGfxLevelESFirst = kGfxLevelES2, kGfxLevelES2First = kGfxLevelES2, kGfxLevelES2Last = kGfxLevelES2,
    kGfxLevelES3, kGfxLevelES3First = kGfxLevelES3,
    kGfxLevelES31,
    kGfxLevelES31AEP, kGfxLevelESLast = kGfxLevelES31AEP, kGfxLevelES3Last = kGfxLevelES31AEP,
    kGfxLevelLegacy, // Default GL context that is given to us by the OS. ONLY USED BY THE LEGACY GL renderer
    kGfxLevelCore32, kGfxLevelCoreFirst = kGfxLevelCore32,
    kGfxLevelCore33,
    kGfxLevelCore40,
    kGfxLevelCore41,
    kGfxLevelCore42,
    kGfxLevelCore43,
    kGfxLevelCore44,
    kGfxLevelCore45, kGfxLevelLast = kGfxLevelCore45, kGfxLevelCoreLast = kGfxLevelCore45
};

enum
{
    kGfxLevelCount = kGfxLevelLast - kGfxLevelFirst + 1
};
#endif//GFX_SUPPORTS_OPENGL_UNIFIED


enum GfxDefaultVertexBufferType
{
	kGfxDefaultVertexBufferBlackWhite,
	kGfxDefaultVertexBufferRedBlue,
	kGfxDefaultVertexBufferCount
};

enum VertexChannelFormat
{
	kChannelFormatFirst = 0,
	kChannelFormatFloat = kChannelFormatFirst,
	kChannelFormatFloat16,
	kChannelFormatColor,
	kChannelFormatByte,
	kChannelFormatUInt32, // Currently only available on GLES 3.0, used for feeding bone indices for skinning.
	kChannelFormatCount
};

#define kMaxVertexStreams 4

struct DrawBuffersRange
{
	DrawBuffersRange()
		: topology(kPrimitiveInvalid)
		, firstIndexByte(0)
		, indexCount(0)
		, baseVertex(0)
		, firstVertex(0)
		, vertexCount(0)
		, instanceCount(0)
		, baseInstanceID(0)
	{}

	GfxPrimitiveType topology;
	UInt32 firstIndexByte;			// Address of first index in bytes
	UInt32 indexCount;				// Used with topology to compute prim count
	UInt32 baseVertex;				// Added to the index when addressing vertices
	UInt32 firstVertex;				// First vertex after baseVertex that's actually used
	UInt32 vertexCount;				// Length of actually used vertex range
	UInt32 instanceCount;			// Specifies the number of instances of the specified range of indices to be rendered.
	UInt32 baseInstanceID;			// Added to the instance ID (either by the API or by shader macros) and accessible as unity_BaseInstanceID.
};

class GfxBuffer;

struct VertexStreamSource
{
	// Don't initialize by default, must be fast to construct at runtime.

	void Reset()
	{
		buffer = NULL;
		stride = 0;
	}

	GfxBuffer* buffer;
	UInt32 stride;
};

class VertexDeclaration;

struct MeshBuffers
{
	// Don't initialize by default, must be fast to construct at runtime.
	// Vertex streams after the count are not guaranteed to have valid values.

	void Reset()
	{
		indexBuffer = NULL;
		vertexStreamCount = 0;
		for (int i = 0; i < kMaxVertexStreams; i++)
			vertexStreams[i].Reset();
		vertexDecl = NULL;
	}

	GfxBuffer* indexBuffer;
	UInt32 vertexStreamCount;
	VertexStreamSource vertexStreams[kMaxVertexStreams];
	VertexDeclaration* vertexDecl;
};