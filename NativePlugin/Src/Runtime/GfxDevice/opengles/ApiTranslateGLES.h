#pragma once

#include "Runtime/Utilities/NonCopyable.h"
#include "Runtime/Utilities/fixed_array.h"
#include "Runtime/GfxDevice/GfxDeviceTypes.h"
#include "ApiFuncGLES.h"
#include "ApiEnumGLES.h"

struct GraphicsCaps;

struct FormatDescGLES
{
	GLenum internalFormat;					// internal format
	GLenum externalFormat;
	GLenum type;
	GLuint flags;							// is compressed, is ETC, is float, is integer, etc. Flags from TextureCap enum
	gl::TextureSwizzleFunc swizzleMode;		// To handle reordering of the component on GPU without CPU memory copy
	GLuint blockSize;						// A block is 1*1 pixel for uncompressed format but n*m pixels for compressed formats. A block is the smallest granularity of a texture, we can't compressed across tile.
};

class TranslateGLES : NonCopyable
{
	static const GLenum kInvalidEnum;

	enum TexFormatTranslation
	{
		kTexFormatTranslationRGB,
		kTexFormatTranslationSRGB,
		kTexFormatTranslationCount
	};

public:
	void Init(const GraphicsCaps & caps, GfxDeviceLevelGL level);

	gl::TexFormat GetFormatGLES(GLenum glesFormat) const;

	GLenum GetBufferTarget(gl::BufferTarget target) const;

	GLenum GetShaderStage(gl::ShaderStage stage) const;
	gl::ShaderStage GetShaderStage(GLenum stage) const;
	const char* GetShaderTitle(gl::ShaderStage stage) const;

	GLenum Func(CompareFunction func) const;
	GLenum StencilOperation(StencilOp op) const;
	GLenum BlendFactor(BlendMode mode) const;
	GLenum BlendEquation(BlendOp equation) const;
	GLenum VertexType(VertexChannelFormat format) const;
	GLenum ObjectType(gl::ObjectType type) const;

	GLenum Topology(GfxPrimitiveType type) const;

	GLenum FramebufferTarget(gl::FramebufferTarget target) const;
	GLenum FramebufferRead(gl::FramebufferRead read) const;
	GLenum FramebufferMask(gl::FramebufferMask Mask) const;
	
	GLenum Enable(gl::EnabledCap cap) const;

	unsigned int VertexArrayKindBitfield(gl::VertexArrayAttribKind kind) const;
	unsigned int VertexArraySizeBitfield(GLint size) const;
	unsigned int VertexArrayTypeBitfield(VertexChannelFormat format) const;
	GLenum MemoryBarrierBits(gl::MemoryBarrierType type) const;

private:
	void InitFramebufferTarget(const GraphicsCaps & caps);
	fixed_array<GLenum, gl::kFramebufferTargetCount> m_FramebufferTarget;

	void InitVertexType(const GraphicsCaps & caps, GfxDeviceLevelGL level);
	fixed_array<GLenum, kChannelFormatCount> m_VertexChannelFormat;

	void InitBufferTarget(const GraphicsCaps & caps, GfxDeviceLevelGL level);
	fixed_array<GLenum, gl::kBufferTargetCount> m_BufferTarget;

	void InitObjectType(const GraphicsCaps & caps);
	fixed_array<GLenum, gl::kObjectTypeCount> m_ObjectType;
};
