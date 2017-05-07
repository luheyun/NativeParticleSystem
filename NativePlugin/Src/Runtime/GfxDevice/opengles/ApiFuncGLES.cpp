#include "PluginPrefix.h"

#include "GfxGetProcAddressGLES.h"
#include "GraphicsCapsGLES.h"
#include "ApiGLES.h"
#include "AssertGLES.h"

#if UNITY_ANDROID
#	define HAS_GLES30_HEADER 1
#	define HAS_GLES31_HEADER 1
#	define HAS_GL_AEP_HEADER 1
#	define HAS_GLCORE_HEADER 0

#else // We load the functions so we rely on our own definitions.
#	define HAS_GLES30_HEADER 1
#	define HAS_GLES31_HEADER 1
#	define HAS_GL_AEP_HEADER 1
#	define HAS_GLCORE_HEADER 1
#endif

void ApiGLES::Load(GfxDeviceLevelGL level)
{
#	if UNITY_APPLE || UNITY_ANDROID || UNITY_WIN
#		define GLESAPI_STRINGIFY(s) GLESAPI_STRINGIFY2(s)
#		define GLESAPI_STRINGIFY2(s) #s
#		define GLES_GET_FUNC(api, func)		do { api.gl ## func = reinterpret_cast<gl::func##Func>(gles::GetProcAddress_core("gl" GLESAPI_STRINGIFY(func))); } while(0)
#	else
#		define GLES_GET_FUNC(api, func)		do { api.gl ## func = reinterpret_cast<gl::func##Func>(::gl ## func); } while(0)
#	endif

	//DebugAssertMsg(IsGfxLevelES(level) || IsGfxLevelCore(level), "OPENGL ERROR: Invalid device level");

	ApiGLES & api = *this;

	GLES_GET_FUNC(api, ActiveTexture);
	GLES_GET_FUNC(api, AttachShader);
	GLES_GET_FUNC(api, BindAttribLocation);
	GLES_GET_FUNC(api, BindBuffer);
	GLES_GET_FUNC(api, BindFramebuffer);
	GLES_GET_FUNC(api, BindRenderbuffer);
	GLES_GET_FUNC(api, BindTexture);
//	GLES_GET_FUNC(api, BlendColor);
	GLES_GET_FUNC(api, BlendEquation);
	GLES_GET_FUNC(api, BlendEquationSeparate);
//	GLES_GET_FUNC(api, BlendFunc);
	GLES_GET_FUNC(api, BlendFuncSeparate);
	GLES_GET_FUNC(api, BufferData);
	GLES_GET_FUNC(api, BufferSubData);
	GLES_GET_FUNC(api, CheckFramebufferStatus);
	GLES_GET_FUNC(api, Clear);
	GLES_GET_FUNC(api, ClearColor);
	GLES_GET_FUNC(api, ClearDepthf);
	GLES_GET_FUNC(api, ClearStencil);
	GLES_GET_FUNC(api, ColorMask);
	GLES_GET_FUNC(api, CompileShader);
	GLES_GET_FUNC(api, CompressedTexImage2D);
	GLES_GET_FUNC(api, CompressedTexSubImage2D);
	GLES_GET_FUNC(api, CopyTexImage2D);
	GLES_GET_FUNC(api, CopyTexSubImage2D);
	GLES_GET_FUNC(api, CreateProgram);
	GLES_GET_FUNC(api, CreateShader);
	GLES_GET_FUNC(api, CullFace);
	GLES_GET_FUNC(api, DeleteBuffers);
	GLES_GET_FUNC(api, DeleteFramebuffers);
	GLES_GET_FUNC(api, DeleteProgram);
	GLES_GET_FUNC(api, DeleteRenderbuffers);
	GLES_GET_FUNC(api, DeleteShader);
	GLES_GET_FUNC(api, DeleteTextures);
	GLES_GET_FUNC(api, DepthFunc);
	GLES_GET_FUNC(api, DepthMask);
//	GLES_GET_FUNC(api, DepthRangef);
//	GLES_GET_FUNC(api, DetachShader);
	GLES_GET_FUNC(api, Disable);
	GLES_GET_FUNC(api, DisableVertexAttribArray);
	GLES_GET_FUNC(api, DrawArrays);
	GLES_GET_FUNC(api, DrawElements);
	GLES_GET_FUNC(api, IsEnabled);
	GLES_GET_FUNC(api, Enable);
	GLES_GET_FUNC(api, EnableVertexAttribArray);
	GLES_GET_FUNC(api, Finish);
	GLES_GET_FUNC(api, Flush);
	GLES_GET_FUNC(api, FramebufferRenderbuffer);
	GLES_GET_FUNC(api, FramebufferTexture2D);
	GLES_GET_FUNC(api, FrontFace);
	GLES_GET_FUNC(api, GenBuffers);
	GLES_GET_FUNC(api, GenerateMipmap);
	GLES_GET_FUNC(api, GenFramebuffers);
	GLES_GET_FUNC(api, GenRenderbuffers);
	GLES_GET_FUNC(api, GenTextures);
	GLES_GET_FUNC(api, GetActiveAttrib);
	GLES_GET_FUNC(api, GetActiveUniform);
//	GLES_GET_FUNC(api, GetAttachedShaders);
	GLES_GET_FUNC(api, GetAttribLocation);
//	GLES_GET_FUNC(api, GetBooleanv);
//	GLES_GET_FUNC(api, GetBufferParameteriv);
	GLES_GET_FUNC(api, GetError);
//	GLES_GET_FUNC(api, GetFloatv);
	GLES_GET_FUNC(api, GetFramebufferAttachmentParameteriv);
	GLES_GET_FUNC(api, GetIntegerv);
	GLES_GET_FUNC(api, GetProgramiv);
	GLES_GET_FUNC(api, GetProgramInfoLog);
	GLES_GET_FUNC(api, ValidateProgram);
//	GLES_GET_FUNC(api, GetRenderbufferParameteriv);
	GLES_GET_FUNC(api, GetShaderiv);
	GLES_GET_FUNC(api, GetShaderSource);
	GLES_GET_FUNC(api, GetShaderInfoLog);
	GLES_GET_FUNC(api, GetShaderPrecisionFormat);
//	GLES_GET_FUNC(api, GetShaderSource);
	GLES_GET_FUNC(api, GetString);
//	GLES_GET_FUNC(api, GetTexParameterfv);
	GLES_GET_FUNC(api, GetTexParameteriv);
//	GLES_GET_FUNC(api, GetUniformfv);
	GLES_GET_FUNC(api, GetUniformiv);
	GLES_GET_FUNC(api, GetUniformLocation);
	GLES_GET_FUNC(api, GetVertexAttribfv);
	GLES_GET_FUNC(api, GetVertexAttribiv);
	GLES_GET_FUNC(api, GetVertexAttribPointerv);
	GLES_GET_FUNC(api, IsEnabled);
//	GLES_GET_FUNC(api, IsFramebuffer);
//	GLES_GET_FUNC(api, IsProgram);
//	GLES_GET_FUNC(api, IsRenderbuffer);
//	GLES_GET_FUNC(api, IsShader);
//	GLES_GET_FUNC(api, IsTexture);
//	GLES_GET_FUNC(api, LineWidth);
	GLES_GET_FUNC(api, LinkProgram);
	GLES_GET_FUNC(api, PixelStorei);
	GLES_GET_FUNC(api, PolygonOffset);
	GLES_GET_FUNC(api, ReadPixels);
//	GLES_GET_FUNC(api, ReleaseShaderCompiler);
	GLES_GET_FUNC(api, RenderbufferStorage);
//	GLES_GET_FUNC(api, SampleCoverage);
	GLES_GET_FUNC(api, Scissor);
//	GLES_GET_FUNC(api, ShaderBinary);
	GLES_GET_FUNC(api, ShaderSource);
	GLES_GET_FUNC(api, StencilFunc);
	GLES_GET_FUNC(api, StencilFuncSeparate);
	GLES_GET_FUNC(api, StencilMask);
//	GLES_GET_FUNC(api, StencilMaskSeparate);
	GLES_GET_FUNC(api, StencilOp);
	GLES_GET_FUNC(api, StencilOpSeparate);
	GLES_GET_FUNC(api, TexImage2D);
	GLES_GET_FUNC(api, TexParameterf);
//	GLES_GET_FUNC(api, TexParameterfv);
	GLES_GET_FUNC(api, TexParameteri);
	GLES_GET_FUNC(api, TexParameteriv);
	GLES_GET_FUNC(api, TexSubImage2D);
//	GLES_GET_FUNC(api, Uniform1f);
	GLES_GET_FUNC(api, Uniform1fv);
	GLES_GET_FUNC(api, Uniform1i);
	GLES_GET_FUNC(api, Uniform1iv);
//	GLES_GET_FUNC(api, Uniform2f);
	GLES_GET_FUNC(api, Uniform2fv);
//	GLES_GET_FUNC(api, Uniform2i);
	GLES_GET_FUNC(api, Uniform2iv);
//	GLES_GET_FUNC(api, Uniform3f);
	GLES_GET_FUNC(api, Uniform3fv);
//	GLES_GET_FUNC(api, Uniform3i);
	GLES_GET_FUNC(api, Uniform3iv);
//	GLES_GET_FUNC(api, Uniform4f);
	GLES_GET_FUNC(api, Uniform4fv);
//	GLES_GET_FUNC(api, Uniform4i);
	GLES_GET_FUNC(api, Uniform4iv);
//	GLES_GET_FUNC(api, UniformMatrix2fv);
	GLES_GET_FUNC(api, UniformMatrix3fv);
	GLES_GET_FUNC(api, UniformMatrix4fv);

	GLES_GET_FUNC(api, UseProgram);
//	GLES_GET_FUNC(api, ValidateProgram);
//	GLES_GET_FUNC(api, VertexAttrib1f);
//	GLES_GET_FUNC(api, VertexAttrib1fv);
//	GLES_GET_FUNC(api, VertexAttrib2f);
//	GLES_GET_FUNC(api, VertexAttrib2fv);
//	GLES_GET_FUNC(api, VertexAttrib3f);
//	GLES_GET_FUNC(api, VertexAttrib3fv);
	GLES_GET_FUNC(api, VertexAttrib4f);
//	GLES_GET_FUNC(api, VertexAttrib4fv);
	GLES_GET_FUNC(api, VertexAttribPointer);
	GLES_GET_FUNC(api, Viewport);

#	if HAS_GLES30_HEADER || HAS_GLCORE_HEADER
	if (IsGfxLevelES(level, kGfxLevelES3) || HAS_GLCORE_HEADER)
	{
		GLES_GET_FUNC(api, GenQueries);
		GLES_GET_FUNC(api, DeleteQueries);

		GLES_GET_FUNC(api, BindVertexArray);
		GLES_GET_FUNC(api, IsVertexArray);
		GLES_GET_FUNC(api, DeleteVertexArrays);
		GLES_GET_FUNC(api, GenVertexArrays);

		GLES_GET_FUNC(api, BeginTransformFeedback);
		GLES_GET_FUNC(api, EndTransformFeedback);
		GLES_GET_FUNC(api, TransformFeedbackVaryings);
		GLES_GET_FUNC(api, BindTransformFeedback);
		GLES_GET_FUNC(api, DeleteTransformFeedbacks);
		GLES_GET_FUNC(api, GenTransformFeedbacks);

		GLES_GET_FUNC(api, TexImage3D);
		GLES_GET_FUNC(api, TexSubImage3D);
		GLES_GET_FUNC(api, CompressedTexSubImage3D);
		GLES_GET_FUNC(api, TexStorage2D);
		GLES_GET_FUNC(api, TexStorage3D);
		GLES_GET_FUNC(api, BlitFramebuffer);
		GLES_GET_FUNC(api, RenderbufferStorageMultisample);
		GLES_GET_FUNC(api, GetStringi);
		GLES_GET_FUNC(api, GetIntegeri_v);
		GLES_GET_FUNC(api, MapBufferRange);
		GLES_GET_FUNC(api, UnmapBuffer);
		GLES_GET_FUNC(api, FlushMappedBufferRange);
		GLES_GET_FUNC(api, InvalidateFramebuffer);
		GLES_GET_FUNC(api, DrawArraysInstanced);
		GLES_GET_FUNC(api, DrawElementsInstanced);
		GLES_GET_FUNC(api, CopyBufferSubData);

		GLES_GET_FUNC(api, DrawBuffers);
		GLES_GET_FUNC(api, ReadBuffer);

		GLES_GET_FUNC(api, FramebufferTextureLayer);
#		if HAS_GL_AEP_HEADER
		GLES_GET_FUNC(api, FramebufferTexture);
#		endif

		GLES_GET_FUNC(api, BindBufferBase);
		GLES_GET_FUNC(api, GetActiveUniformsiv);
		GLES_GET_FUNC(api, GetUniformBlockIndex);
		GLES_GET_FUNC(api, GetActiveUniformBlockiv);
		GLES_GET_FUNC(api, GetActiveUniformBlockName);
		GLES_GET_FUNC(api, UniformBlockBinding);
		GLES_GET_FUNC(api, VertexAttribIPointer);

		GLES_GET_FUNC(api, GetProgramBinary);
		GLES_GET_FUNC(api, ProgramBinary);
		GLES_GET_FUNC(api, ProgramParameteri);

		GLES_GET_FUNC(api, GenSamplers);
		GLES_GET_FUNC(api, DeleteSamplers);
		GLES_GET_FUNC(api, BindSampler);
		GLES_GET_FUNC(api, SamplerParameteri);

		GLES_GET_FUNC(api, GetInternalformativ);

		GLES_GET_FUNC(api, FenceSync);
		GLES_GET_FUNC(api, ClientWaitSync);
		GLES_GET_FUNC(api, DeleteSync);
	}
#	endif

#	if HAS_GLES31_HEADER || HAS_GLCORE_HEADER
	if (IsGfxLevelES(level, kGfxLevelES31) || HAS_GLCORE_HEADER)
	{
	//	GLES_GET_FUNC(api, ProgramUniform1f);
		GLES_GET_FUNC(api, ProgramUniform1fv);
	//	GLES_GET_FUNC(api, ProgramUniform1i);
		GLES_GET_FUNC(api, ProgramUniform1iv);
	//	GLES_GET_FUNC(api, ProgramUniform2f);
		GLES_GET_FUNC(api, ProgramUniform2fv);
	//	GLES_GET_FUNC(api, ProgramUniform2i);
		GLES_GET_FUNC(api, ProgramUniform2iv);
	//	GLES_GET_FUNC(api, ProgramUniform3f);
		GLES_GET_FUNC(api, ProgramUniform3fv);
	//	GLES_GET_FUNC(api, ProgramUniform3i);
		GLES_GET_FUNC(api, ProgramUniform3iv);
	//	GLES_GET_FUNC(api, ProgramUniform4f);
		GLES_GET_FUNC(api, ProgramUniform4fv);
	//	GLES_GET_FUNC(api, ProgramUniform4i);
		GLES_GET_FUNC(api, ProgramUniform4iv);
		GLES_GET_FUNC(api, ProgramUniformMatrix2fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix3fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix4fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix2x3fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix3x2fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix2x4fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix4x2fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix3x4fv);
		GLES_GET_FUNC(api, ProgramUniformMatrix4x3fv);
	//	GLES_GET_FUNC(api, ProgramUniform1ui);
	//	GLES_GET_FUNC(api, ProgramUniform2ui);
	//	GLES_GET_FUNC(api, ProgramUniform3ui);
	//	GLES_GET_FUNC(api, ProgramUniform4ui);
		GLES_GET_FUNC(api, ProgramUniform1uiv);
		GLES_GET_FUNC(api, ProgramUniform2uiv);
		GLES_GET_FUNC(api, ProgramUniform3uiv);
		GLES_GET_FUNC(api, ProgramUniform4uiv);

		GLES_GET_FUNC(api, BindImageTexture);

		GLES_GET_FUNC(api, DispatchCompute);
		GLES_GET_FUNC(api, DispatchComputeIndirect);

		GLES_GET_FUNC(api, GetProgramInterfaceiv);
		GLES_GET_FUNC(api, GetProgramResourceName);
		GLES_GET_FUNC(api, GetProgramResourceiv);

		GLES_GET_FUNC(api, DrawArraysIndirect);

		// Avoid name conflict with winnt.h
#		undef MemoryBarrier
		GLES_GET_FUNC(api, MemoryBarrier);
	}
#	endif

#define INTERNAL_GLES_GET_NAMED(api, func, name, force)						\
	do																		\
	{																		\
		typedef gl::func##Func FuncPtrType;									\
		FuncPtrType& fptr = api.gl ## func;									\
		if (!fptr || force)													\
		{																	\
			if (void* const loadedFunc = gles::GetProcAddress(name))		\
			{																\
				fptr = reinterpret_cast<FuncPtrType>(loadedFunc);			\
			}																\
		}																	\
	} while(0)

#define GLES_GET_NAMED(api, func, name) INTERNAL_GLES_GET_NAMED(api, func, name, false)
#define GLES_GET_NAMED_OVERRIDE(api, func, name) INTERNAL_GLES_GET_NAMED(api, func, name, true)

#	if HAS_GL_AEP_HEADER || HAS_GLCORE_HEADER
	if (IsGfxLevelES(level, kGfxLevelES31AEP) || HAS_GLCORE_HEADER)
	{
		GLES_GET_FUNC(api, TexStorage2DMultisample);
		GLES_GET_FUNC(api, PatchParameteri);
		GLES_GET_FUNC(api, PatchParameterfv);
		GLES_GET_FUNC(api, CopyImageSubData);
	}
#	endif

#	if CAN_HAVE_DIRECT_STATE_ACCESS && HAS_GLCORE_HEADER
	{
		GLES_GET_FUNC(api, CreateTextures);
		GLES_GET_FUNC(api, BindTextureUnit);
		GLES_GET_FUNC(api, TextureStorage1D);
		GLES_GET_FUNC(api, TextureStorage2D);
		GLES_GET_FUNC(api, TextureStorage2DMultisample);
		GLES_GET_FUNC(api, TextureStorage3D);
		GLES_GET_FUNC(api, TextureStorage3DMultisample);
		GLES_GET_FUNC(api, TextureSubImage1D);
		GLES_GET_FUNC(api, TextureSubImage2D);
		GLES_GET_FUNC(api, TextureSubImage3D);
		GLES_GET_FUNC(api, TextureBuffer);
		GLES_GET_FUNC(api, TextureBufferRange);
		GLES_GET_FUNC(api, TextureParameteri);
		GLES_GET_FUNC(api, TextureParameteriv);
		GLES_GET_FUNC(api, TextureParameteriiv);
		GLES_GET_FUNC(api, TextureParameteriuiv);
		GLES_GET_FUNC(api, TextureParameterf);
		GLES_GET_FUNC(api, TextureParameterfv);
		GLES_GET_FUNC(api, CompressedTextureSubImage1D);
		GLES_GET_FUNC(api, CompressedTextureSubImage2D);
		GLES_GET_FUNC(api, CompressedTextureSubImage3D);
		GLES_GET_FUNC(api, CopyTextureSubImage1D);
		GLES_GET_FUNC(api, CopyTextureSubImage2D);
		GLES_GET_FUNC(api, CopyTextureSubImage3D);
		GLES_GET_FUNC(api, GenerateTextureMipmap);
		GLES_GET_FUNC(api, GetCompressedTextureImage);
		GLES_GET_FUNC(api, GetTextureImage);
		GLES_GET_FUNC(api, GetTextureLevelParameterfv);
		GLES_GET_FUNC(api, GetTextureLevelParameteriv);
		GLES_GET_FUNC(api, GetTextureLevelParameteriiv);
		GLES_GET_FUNC(api, GetTextureLevelParameteriuiv);
		GLES_GET_FUNC(api, GetTextureParameterfv);
		GLES_GET_FUNC(api, GetTextureParameteriv);

		GLES_GET_FUNC(api, BlitNamedFramebuffer);
		GLES_GET_FUNC(api, ClearNamedFramebufferi);
		GLES_GET_FUNC(api, ClearNamedFramebufferfv);
		GLES_GET_FUNC(api, ClearNamedFramebufferiv);
		GLES_GET_FUNC(api, ClearNamedFramebufferuiv);
		GLES_GET_FUNC(api, CheckNamedFramebufferStatus);
		GLES_GET_FUNC(api, GetNamedFramebufferParameteriv);
		GLES_GET_FUNC(api, GetNamedFramebufferAttachmentParameteriv);
		GLES_GET_FUNC(api, InvalidateNamedFramebufferData);
		GLES_GET_FUNC(api, InvalidateNamedFramebufferSubData);
		GLES_GET_FUNC(api, NamedFramebufferDrawBuffer);
		GLES_GET_FUNC(api, NamedFramebufferDrawBuffers);
		GLES_GET_FUNC(api, NamedFramebufferParameteri);
		GLES_GET_FUNC(api, NamedFramebufferReadBuffer);
		GLES_GET_FUNC(api, NamedFramebufferRenderbuffer);
		GLES_GET_FUNC(api, NamedFramebufferTexture);
		GLES_GET_FUNC(api, NamedFramebufferTextureLayer);

		GLES_GET_FUNC(api, CreateBuffers);
		GLES_GET_FUNC(api, ClearNamedBufferData);
		GLES_GET_FUNC(api, ClearNamedBufferSubData);
		GLES_GET_FUNC(api, NamedBufferStorage);
		GLES_GET_FUNC(api, NamedBufferData);
		GLES_GET_FUNC(api, NamedBufferSubData);
		GLES_GET_FUNC(api, CopyNamedBufferSubData);
		GLES_GET_FUNC(api, FlushMappedNamedBufferRange);
		GLES_GET_FUNC(api, MapNamedBuffer);
		GLES_GET_FUNC(api, MapNamedBufferRange);
		GLES_GET_FUNC(api, UnmapNamedBuffer);
		GLES_GET_FUNC(api, GetNamedBufferSubData);
		GLES_GET_FUNC(api, GetNamedBufferParameteri64v);
		GLES_GET_FUNC(api, GetNamedBufferParameteriv);
		GLES_GET_FUNC(api, GetNamedBufferPointerv);

		GLES_GET_FUNC(api, CreateRenderbuffers);
		GLES_GET_FUNC(api, NamedRenderbufferStorage);
		GLES_GET_FUNC(api, NamedRenderbufferStorageMultisample);
		GLES_GET_FUNC(api, GetNamedRenderbufferParameteriv);

		GLES_GET_FUNC(api, CreateProgramPipelines);
		GLES_GET_FUNC(api, CreateSamplers);

		GLES_GET_FUNC(api, CreateTransformFeedbacks);
		GLES_GET_FUNC(api, TransformFeedbackBufferBase);
		GLES_GET_FUNC(api, TransformFeedbackBufferRange);
		GLES_GET_FUNC(api, GetTransformFeedbacki64_v);
		GLES_GET_FUNC(api, GetTransformFeedbacki_v);
		GLES_GET_FUNC(api, GetTransformFeedbackiv);

		GLES_GET_FUNC(api, CreateVertexArrays);
		GLES_GET_FUNC(api, DisableVertexArrayAttrib);
		GLES_GET_FUNC(api, EnableVertexArrayAttrib);
		GLES_GET_FUNC(api, GetVertexArrayIndexed64iv);
		GLES_GET_FUNC(api, GetVertexArrayIndexediv);
		GLES_GET_FUNC(api, GetVertexArrayiv);
		GLES_GET_FUNC(api, VertexArrayAttribBinding);
		GLES_GET_FUNC(api, VertexArrayAttribFormat);
		GLES_GET_FUNC(api, VertexArrayAttribIFormat);
		GLES_GET_FUNC(api, VertexArrayAttribLFormat);
		GLES_GET_FUNC(api, VertexArrayBindingDivisor);
		GLES_GET_FUNC(api, VertexArrayElementBuffer);
		GLES_GET_FUNC(api, VertexArrayVertexBuffer);
		GLES_GET_FUNC(api, VertexArrayVertexBuffers);

		GLES_GET_NAMED(api, TexturePageCommitment, "glTexturePageCommitmentARB");
		GLES_GET_NAMED(api, TexturePageCommitment, "glTexturePageCommitmentEXT");

		GLES_GET_NAMED(api, NamedBufferStorage, "glNamedBufferStorage");
	}
#	endif//CAN_HAVE_DIRECT_STATE_ACCESS && HAS_GLCORE_HEADER

	// Desktop GL helpers
#	if HAS_GLCORE_HEADER
	{
		GLES_GET_FUNC(api, QueryCounter);
		GLES_GET_FUNC(api, GetQueryObjectui64v);
		GLES_GET_FUNC(api, DrawBuffer);
		GLES_GET_FUNC(api, PolygonMode);
		GLES_GET_FUNC(api, ClearDepth);
		GLES_GET_FUNC(api, DrawElementsBaseVertex);
		GLES_GET_FUNC(api, DrawElementsInstancedBaseVertex);
		GLES_GET_FUNC(api, ClearBufferData);
		GLES_GET_FUNC(api, ClearBufferSubData);
	}
#	endif//HAS_GLCORE_HEADER

	#undef GLES_GET_FUNC

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_EXT_buffer_storage") || api.QueryExtension("GL_ARB_buffer_storage"))
	{
		GLES_GET_NAMED(api, BufferStorage, "glBufferStorage");
		GLES_GET_NAMED(api, BufferStorage, "glBufferStorageEXT");
	}

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_EXT_multisampled_render_to_texture"))
	{
		// In ARM Mali driver glRenderbufferStorageMultisample and glRenderbufferStorageMultisampleEXT are two distinct functions
		// and glRenderbufferStorageMultisample cannot be combined with glFramebufferTexture2DMultisampleEXT
		// so we override the ES3 function with the EXT function.
		GLES_GET_NAMED_OVERRIDE(api, RenderbufferStorageMultisample, "glRenderbufferStorageMultisampleEXT");
		GLES_GET_NAMED(api, FramebufferTexture2DMultisampleEXT, "glFramebufferTexture2DMultisampleEXT");
	}

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_IMG_multisampled_render_to_texture"))
	{
		GLES_GET_NAMED(api, RenderbufferStorageMultisample, "glRenderbufferStorageMultisampleIMG");
		GLES_GET_NAMED(api, FramebufferTexture2DMultisampleEXT, "glFramebufferTexture2DMultisampleIMG");
	}

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_APPLE_framebuffer_multisample"))
	{
		GLES_GET_NAMED(api, RenderbufferStorageMultisample, "glRenderbufferStorageMultisampleAPPLE");
		GLES_GET_NAMED(api, ResolveMultisampleFramebufferAPPLE, "glResolveMultisampleFramebufferAPPLE");
	}

	if (HAS_GLCORE_HEADER || (IsGfxLevelES2(level) && api.QueryExtension("GL_NV_framebuffer_multisample") && api.QueryExtension("GL_NV_framebuffer_blit")))
	{
		GLES_GET_NAMED(api, RenderbufferStorageMultisample, "glRenderbufferStorageMultisampleNV");
	}

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_KHR_debug"))
	{
		if (IsGfxLevelES(level))
		{
			GLES_GET_NAMED(api, DebugMessageControl, "glDebugMessageControlKHR");
			GLES_GET_NAMED(api, DebugMessageCallback, "glDebugMessageCallbackKHR");
			GLES_GET_NAMED(api, DebugMessageInsert, "glDebugMessageInsertKHR");
			GLES_GET_NAMED(api, ObjectLabel, "glObjectLabelKHR");
			GLES_GET_NAMED(api, GetObjectLabel, "glGetObjectLabelKHR");
			GLES_GET_NAMED(api, PushDebugGroup, "glPushDebugGroupKHR");
			GLES_GET_NAMED(api, PopDebugGroup, "glPopDebugGroupKHR");
		}
		
		if (IsGfxLevelCore(level))
		{
			GLES_GET_NAMED(api, DebugMessageControl, "glDebugMessageControl");
			GLES_GET_NAMED(api, DebugMessageCallback, "glDebugMessageCallback");
			GLES_GET_NAMED(api, DebugMessageInsert, "glDebugMessageInsert");
			GLES_GET_NAMED(api, ObjectLabel, "glObjectLabel");
			GLES_GET_NAMED(api, GetObjectLabel, "glGetObjectLabel");
			GLES_GET_NAMED(api, PushDebugGroup, "glPushDebugGroup");
			GLES_GET_NAMED(api, PopDebugGroup, "glPopDebugGroup");
		}
	}
	
	// Some devices (Tegra 4, Tegra 3) report GL_KHR_debug as supported but the entrypoints are missing.
	// Therefore all the extension alternatives must also be checked despite of GL_KHR_debug query:
	// fetch ARB versions only if KHR versions are not properly present
	if (HAS_GLCORE_HEADER || ((!api.glDebugMessageControl || ! api.glDebugMessageCallback) && api.QueryExtension("GL_ARB_debug_output")))
	{
		GLES_GET_NAMED(api, DebugMessageControl, "glDebugMessageControlARB");
		GLES_GET_NAMED(api, DebugMessageCallback, "glDebugMessageCallbackARB");
		GLES_GET_NAMED(api, DebugMessageInsert, "glDebugMessageInsertARB");
	}

	// dedicated EXT entrypoint -> always fetch if available
	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_EXT_debug_marker"))
	{
		GLES_GET_NAMED(api, PushGroupMarkerEXT, "glPushGroupMarkerEXT");
		GLES_GET_NAMED(api, PopGroupMarkerEXT, "glPopGroupMarkerEXT");
	}

	// dedicated EXT entrypoint -> always fetch if available
	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_EXT_debug_label"))
	{
		GLES_GET_NAMED(api, LabelObjectEXT, "glLabelObjectEXT");
		GLES_GET_NAMED(api, GetObjectLabelEXT, "glGetObjectLabelEXT");
	}

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_EXT_disjoint_timer_query"))
	{
		GLES_GET_NAMED(api, GenQueries, "glGenQueriesEXT");
		GLES_GET_NAMED(api, DeleteQueries, "glDeleteQueriesEXT");
		GLES_GET_NAMED(api, QueryCounter, "glQueryCounterEXT");
		GLES_GET_NAMED(api, GetQueryObjectui64v, "glGetQueryObjectui64vEXT");
	}

	if (HAS_GLCORE_HEADER || api.QueryExtension("GL_NV_timer_query"))
	{
		GLES_GET_NAMED(api, GenQueries, "glGenQueriesEXT");
		GLES_GET_NAMED(api, DeleteQueries, "glDeleteQueriesEXT");
		GLES_GET_NAMED(api, QueryCounter, "glQueryCounterNV");
		GLES_GET_NAMED(api, GetQueryObjectui64v, "glGetQueryObjectui64vNV");
	}

	if (HAS_GLCORE_HEADER || IsGfxLevelES2(level))
	{
		if (api.QueryExtension("GL_OES_texture_3D"))
		{
			GLES_GET_NAMED(api, TexImage3D, "glTexImage3DOES");
			GLES_GET_NAMED(api, TexSubImage3D, "glTexSubImage3DOES");
		}

		if (api.QueryExtension("GL_OES_vertex_array_object"))
		{
			GLES_GET_NAMED(api, BindVertexArray, "glBindVertexArrayOES");
			GLES_GET_NAMED(api, DeleteVertexArrays, "glDeleteVertexArraysOES");
			GLES_GET_NAMED(api, GenVertexArrays, "glGenVertexArraysOES");
		}

		if (api.QueryExtension("GL_EXT_draw_buffers"))
		{
			GLES_GET_NAMED(api, DrawBuffers, "glDrawBuffersEXT");
		}
		else if (api.QueryExtension("GL_NV_draw_buffers"))
		{
			GLES_GET_NAMED(api, DrawBuffers, "glDrawBuffersNV");
		}

		if (api.QueryExtension("GL_NV_read_buffer"))
			GLES_GET_NAMED(api, ReadBuffer, "glReadBufferNV");

		if (api.QueryExtension("GL_NV_framebuffer_blit"))
			GLES_GET_NAMED(api, BlitFramebuffer, "glBlitFramebufferNV");

		if (api.QueryExtension("GL_EXT_discard_framebuffer"))
			GLES_GET_NAMED(api, InvalidateFramebuffer, "glDiscardFramebufferEXT");

		if (api.QueryExtension("GL_EXT_map_buffer_range"))
		{
			GLES_GET_NAMED(api, MapBufferRange, "glMapBufferRangeEXT");
			GLES_GET_NAMED(api, FlushMappedBufferRange, "glFlushMappedBufferRangeEXT");
			GLES_GET_NAMED(api, UnmapBuffer, "glUnmapBufferOES");
			GLES_GET_NAMED(api, UnmapBuffer, "glUnmapBufferEXT");
		}

		if (api.QueryExtension("GL_OES_map_buffer"))
		{
			GLES_GET_NAMED(api, MapBuffer, "glMapBufferOES");
			GLES_GET_NAMED(api, UnmapBuffer, "glUnmapBufferOES");
		}

		if (api.QueryExtension("GL_OES_get_program_binary"))
		{
			GLES_GET_NAMED(api, GetProgramBinary, "glGetProgramBinaryOES");
			GLES_GET_NAMED(api, ProgramBinary, "glProgramBinaryOES");
		}
	}

	if (HAS_GLCORE_HEADER || IsGfxLevelES(level))
	{
		if (api.QueryExtension("GL_OES_copy_image"))
		{
			GLES_GET_NAMED(api, CopyImageSubData, "glCopyImageSubDataOES");
		}
		else if (api.QueryExtension("GL_EXT_copy_image"))
		{
			GLES_GET_NAMED(api, CopyImageSubData, "glCopyImageSubDataEXT");
		}

		if (api.QueryExtension("GL_OES_tessellation_shader"))
		{
			GLES_GET_NAMED(api, PatchParameteri, "glPatchParameteriOES");
			GLES_GET_NAMED(api, PatchParameterfv, "glPatchParameterfvOES");
		}
		else if (api.QueryExtension("GL_EXT_tessellation_shader"))
		{
			GLES_GET_NAMED(api, PatchParameteri, "glPatchParameteriEXT");
			GLES_GET_NAMED(api, PatchParameterfv, "glPatchParameterfvEXT");
		}

		if (api.QueryExtension("GL_OES_draw_elements_base_vertex"))
		{
			GLES_GET_NAMED(api, DrawElementsBaseVertex, "glDrawElementsBaseVertexOES");
			GLES_GET_NAMED(api, DrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertexOES");
		}
		else if (api.QueryExtension("GL_EXT_draw_elements_base_vertex"))
		{
			GLES_GET_NAMED(api, DrawElementsBaseVertex, "glDrawElementsBaseVertexEXT");
			GLES_GET_NAMED(api, DrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertexEXT");
		}

		if (api.QueryExtension("GL_ARB_sparse_texture"))
		{
			GLES_GET_NAMED(api, TexPageCommitment, "glTexPageCommitmentARB");
		}
		else if (api.QueryExtension("GL_EXT_sparse_texture"))
		{
			GLES_GET_NAMED(api, TexPageCommitment, "glTexPageCommitmentEXT");
		}

		if (api.QueryExtension("GL_EXT_texture_storage"))
		{
			GLES_GET_NAMED(api, TexStorage2D, "glTexStorage2DEXT");
			GLES_GET_NAMED(api, TexStorage3D, "glTexStorage3DEXT");
		}

		if (api.QueryExtension("GL_KHR_blend_equation_advanced"))
		{
			GLES_GET_NAMED(api, BlendBarrier, "glBlendBarrierKHR");
		}
		else if (api.QueryExtension("GL_NV_blend_equation_advanced"))
		{
			GLES_GET_NAMED(api, BlendBarrier, "glBlendBarrierNV");
		}
	}

#	undef GLES_GET_NAMED
}
