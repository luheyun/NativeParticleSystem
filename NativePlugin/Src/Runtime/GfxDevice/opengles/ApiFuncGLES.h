#pragma once

#include "ApiTypeGLES.h"

#if UNITY_WIN
#	define GLES_APIENTRY WINAPI
#elif defined(__NDK_FPABI__)
#	define GLES_APIENTRY __NDK_FPABI__
#else
#	define GLES_APIENTRY
#endif

namespace gl
{
	// -- ES2 Functions declaration --
	typedef void			(GLES_APIENTRY *ActiveTextureFunc)(GLenum texture);
	typedef void			(GLES_APIENTRY *AttachShaderFunc)(GLuint program, GLuint shader);
	typedef void			(GLES_APIENTRY *BindAttribLocationFunc)(GLuint program, GLuint index, const GLchar* name);
	typedef void			(GLES_APIENTRY *BindBufferFunc)(GLenum target, GLuint buffer);
	typedef void			(GLES_APIENTRY *BindFramebufferFunc)(GLenum target, GLuint framebuffer);
	typedef void			(GLES_APIENTRY *BindRenderbufferFunc)(GLenum target, GLuint renderbuffer);
	typedef void			(GLES_APIENTRY *BindTextureFunc)(GLenum target, GLuint texture);
	typedef void			(GLES_APIENTRY *BlendColorFunc)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	typedef void			(GLES_APIENTRY *BlendEquationFunc)( GLenum mode );
	typedef void			(GLES_APIENTRY *BlendEquationSeparateFunc)(GLenum modeRGB, GLenum modeAlpha);
	typedef void			(GLES_APIENTRY *BlendFuncFunc)(GLenum sfactor, GLenum dfactor);
	typedef void			(GLES_APIENTRY *BlendFuncSeparateFunc)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	typedef void			(GLES_APIENTRY *BufferDataFunc)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
	typedef void			(GLES_APIENTRY *BufferSubDataFunc)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
	typedef GLenum			(GLES_APIENTRY *CheckFramebufferStatusFunc)(GLenum target);
	typedef void			(GLES_APIENTRY *ClearFunc)(GLbitfield mask);
	typedef void			(GLES_APIENTRY *ClearColorFunc)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	typedef void			(GLES_APIENTRY *ClearDepthfFunc)(GLclampf depth);
	typedef void			(GLES_APIENTRY *ClearStencilFunc)(GLint s);
	typedef void			(GLES_APIENTRY *ColorMaskFunc)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	typedef void			(GLES_APIENTRY *CompileShaderFunc)(GLuint shader);
	typedef void			(GLES_APIENTRY *CompressedTexImage2DFunc)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void			(GLES_APIENTRY *CompressedTexSubImage2DFunc)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void			(GLES_APIENTRY *CopyTexImage2DFunc)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	typedef void			(GLES_APIENTRY *CopyTexSubImage2DFunc)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef GLuint			(GLES_APIENTRY *CreateProgramFunc)(void);
	typedef GLuint			(GLES_APIENTRY *CreateShaderFunc)(GLenum type);
	typedef void			(GLES_APIENTRY *CullFaceFunc)(GLenum mode);
	typedef void			(GLES_APIENTRY *DeleteBuffersFunc)(GLsizei n, const GLuint* buffers);
	typedef void			(GLES_APIENTRY *DeleteFramebuffersFunc)(GLsizei n, const GLuint* framebuffers);
	typedef void			(GLES_APIENTRY *DeleteProgramFunc)(GLuint program);
	typedef void			(GLES_APIENTRY *DeleteRenderbuffersFunc)(GLsizei n, const GLuint* renderbuffers);
	typedef void			(GLES_APIENTRY *DeleteShaderFunc)(GLuint shader);
	typedef void			(GLES_APIENTRY *DeleteTexturesFunc)(GLsizei n, const GLuint* textures);
	typedef void			(GLES_APIENTRY *DepthFuncFunc)(GLenum func);
	typedef void			(GLES_APIENTRY *DepthMaskFunc)(GLboolean flag);
	typedef void			(GLES_APIENTRY *DepthRangefFunc)(GLclampf zNear, GLclampf zFar);
	typedef void			(GLES_APIENTRY *DetachShaderFunc)(GLuint program, GLuint shader);
	typedef void			(GLES_APIENTRY *DisableFunc)(GLenum cap);
	typedef void			(GLES_APIENTRY *DisableVertexAttribArrayFunc)(GLuint index);
	typedef void			(GLES_APIENTRY *DrawArraysFunc)(GLenum mode, GLint first, GLsizei count);
	typedef void			(GLES_APIENTRY *DrawElementsFunc)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
	typedef void			(GLES_APIENTRY *EnableFunc)(GLenum cap);
	typedef void			(GLES_APIENTRY *EnableVertexAttribArrayFunc)(GLuint index);
	typedef void			(GLES_APIENTRY *FinishFunc)(void);
	typedef void			(GLES_APIENTRY *FlushFunc)(void);
	typedef void			(GLES_APIENTRY *FramebufferRenderbufferFunc)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	typedef void			(GLES_APIENTRY *FramebufferTexture2DFunc)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	typedef void			(GLES_APIENTRY *FrontFaceFunc)(GLenum mode);
	typedef void			(GLES_APIENTRY *GenBuffersFunc)(GLsizei n, GLuint* buffers);
	typedef void			(GLES_APIENTRY *GenerateMipmapFunc)(GLenum target);
	typedef void			(GLES_APIENTRY *GenFramebuffersFunc)(GLsizei n, GLuint* framebuffers);
	typedef void			(GLES_APIENTRY *GenRenderbuffersFunc)(GLsizei n, GLuint* renderbuffers);
	typedef void			(GLES_APIENTRY *GenTexturesFunc)(GLsizei n, GLuint* textures);
	typedef void			(GLES_APIENTRY *GetActiveAttribFunc)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
	typedef void			(GLES_APIENTRY *GetActiveUniformFunc)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
	typedef void			(GLES_APIENTRY *GetAttachedShadersFunc)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
	typedef GLint			(GLES_APIENTRY *GetAttribLocationFunc)(GLuint program, const GLchar* name);
	typedef void			(GLES_APIENTRY *GetBooleanvFunc)(GLenum pname, GLboolean* params);
	typedef void			(GLES_APIENTRY *GetBufferParameterivFunc)(GLenum target, GLenum pname, GLint* params);
	typedef GLenum			(GLES_APIENTRY *GetErrorFunc)(void);
	typedef void			(GLES_APIENTRY *GetFloatvFunc)(GLenum pname, GLfloat* params);
	typedef void			(GLES_APIENTRY *GetFramebufferAttachmentParameterivFunc)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetIntegervFunc)(GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetProgramivFunc)(GLuint program, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetProgramInfoLogFunc)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
	typedef void			(GLES_APIENTRY *GetRenderbufferParameterivFunc)(GLenum target, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetShaderivFunc)(GLuint shader, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetShaderInfoLogFunc)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
	typedef void			(GLES_APIENTRY *GetShaderPrecisionFormatFunc)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
	typedef void			(GLES_APIENTRY *GetShaderSourceFunc)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
	typedef const GLubyte*	(GLES_APIENTRY *GetStringFunc)(GLenum name);
	typedef void			(GLES_APIENTRY *GetTexParameterfvFunc)(GLenum target, GLenum pname, GLfloat* params);
	typedef void			(GLES_APIENTRY *GetTexParameterivFunc)(GLenum target, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetUniformfvFunc)(GLuint program, GLint location, GLfloat* params);
	typedef void			(GLES_APIENTRY *GetUniformivFunc)(GLuint program, GLint location, GLint* params);
	typedef GLint			(GLES_APIENTRY *GetUniformLocationFunc)(GLuint program, const GLchar* name);
	typedef void			(GLES_APIENTRY *GetVertexAttribfvFunc)(GLuint index, GLenum pname, GLfloat* params);
	typedef void			(GLES_APIENTRY *GetVertexAttribivFunc)(GLuint index, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetVertexAttribPointervFunc)(GLuint index, GLenum pname, GLvoid** pointer);
	typedef GLboolean		(GLES_APIENTRY *IsEnabledFunc)(GLenum cap);
	typedef GLboolean		(GLES_APIENTRY *IsFramebufferFunc)(GLuint framebuffer);
	typedef GLboolean		(GLES_APIENTRY *IsProgramFunc)(GLuint program);
	typedef GLboolean		(GLES_APIENTRY *IsRenderbufferFunc)(GLuint renderbuffer);
	typedef GLboolean		(GLES_APIENTRY *IsShaderFunc)(GLuint shader);
	typedef GLboolean		(GLES_APIENTRY *IsTextureFunc)(GLuint texture);
	typedef void			(GLES_APIENTRY *LineWidthFunc)(GLfloat width);
	typedef void			(GLES_APIENTRY *LinkProgramFunc)(GLuint program);
	typedef void			(GLES_APIENTRY *PixelStoreiFunc)(GLenum pname, GLint param);
	typedef void			(GLES_APIENTRY *PolygonOffsetFunc)(GLfloat factor, GLfloat units);
	typedef void			(GLES_APIENTRY *ReadPixelsFunc)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
	typedef void			(GLES_APIENTRY *ReleaseShaderCompilerFunc)(void);
	typedef void			(GLES_APIENTRY *RenderbufferStorageFunc)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void			(GLES_APIENTRY *SampleCoverageFunc)(GLclampf value, GLboolean invert);
	typedef void			(GLES_APIENTRY *ScissorFunc)(GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void			(GLES_APIENTRY *ShaderBinaryFunc)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
	typedef void			(GLES_APIENTRY *ShaderSourceFunc)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
	typedef void			(GLES_APIENTRY *StencilFuncFunc)(GLenum func, GLint ref, GLuint mask);
	typedef void			(GLES_APIENTRY *StencilFuncSeparateFunc)(GLenum face, GLenum func, GLint ref, GLuint mask);
	typedef void			(GLES_APIENTRY *StencilMaskFunc)(GLuint mask);
	typedef void			(GLES_APIENTRY *StencilMaskSeparateFunc)(GLenum face, GLuint mask);
	typedef void			(GLES_APIENTRY *StencilOpFunc)(GLenum fail, GLenum zfail, GLenum zpass);
	typedef void			(GLES_APIENTRY *StencilOpSeparateFunc)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
	typedef void			(GLES_APIENTRY *TexImage2DFunc)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void			(GLES_APIENTRY *TexImage2DMultisampleFunc)(GLenum target, GLint samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	typedef void			(GLES_APIENTRY *TexParameterfFunc)(GLenum target, GLenum pname, GLfloat param);
	typedef void			(GLES_APIENTRY *TexParameterfvFunc)(GLenum target, GLenum pname, const GLfloat* params);
	typedef void			(GLES_APIENTRY *TexParameteriFunc)(GLenum target, GLenum pname, GLint param);
	typedef void			(GLES_APIENTRY *TexParameterivFunc)(GLenum target, GLenum pname, const GLint* params);
	typedef void			(GLES_APIENTRY *TexSubImage2DFunc)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void			(GLES_APIENTRY *Uniform1fFunc)(GLint location, GLfloat x);
	typedef void			(GLES_APIENTRY *Uniform1fvFunc)(GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *Uniform1iFunc)(GLint location, GLint x);
	typedef void			(GLES_APIENTRY *Uniform1ivFunc)(GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *Uniform2fFunc)(GLint location, GLfloat x, GLfloat y);
	typedef void			(GLES_APIENTRY *Uniform2fvFunc)(GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *Uniform2iFunc)(GLint location, GLint x, GLint y);
	typedef void			(GLES_APIENTRY *Uniform2ivFunc)(GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *Uniform3fFunc)(GLint location, GLfloat x, GLfloat y, GLfloat z);
	typedef void			(GLES_APIENTRY *Uniform3fvFunc)(GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *Uniform3iFunc)(GLint location, GLint x, GLint y, GLint z);
	typedef void			(GLES_APIENTRY *Uniform3ivFunc)(GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *Uniform4fFunc)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void			(GLES_APIENTRY *Uniform4fvFunc)(GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *Uniform4iFunc)(GLint location, GLint x, GLint y, GLint z, GLint w);
	typedef void			(GLES_APIENTRY *Uniform4ivFunc)(GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *UniformMatrix2fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix3fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix4fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UseProgramFunc)(GLuint program);
	typedef void			(GLES_APIENTRY *ValidateProgramFunc)(GLuint program);
	typedef void			(GLES_APIENTRY *VertexAttrib1fFunc)(GLuint indx, GLfloat x);
	typedef void			(GLES_APIENTRY *VertexAttrib1fvFunc)(GLuint indx, const GLfloat* values);
	typedef void			(GLES_APIENTRY *VertexAttrib2fFunc)(GLuint indx, GLfloat x, GLfloat y);
	typedef void			(GLES_APIENTRY *VertexAttrib2fvFunc)(GLuint indx, const GLfloat* values);
	typedef void			(GLES_APIENTRY *VertexAttrib3fFunc)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
	typedef void			(GLES_APIENTRY *VertexAttrib3fvFunc)(GLuint indx, const GLfloat* values);
	typedef void			(GLES_APIENTRY *VertexAttrib4fFunc)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void			(GLES_APIENTRY *VertexAttrib4fvFunc)(GLuint indx, const GLfloat* values);
	typedef void			(GLES_APIENTRY *VertexAttribPointerFunc)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	typedef void			(GLES_APIENTRY *ViewportFunc)(GLint x, GLint y, GLsizei width, GLsizei height);

	// -- ES 3.0 declarations --
	typedef void			(GLES_APIENTRY *ReadBufferFunc)(GLenum mode);
	typedef void			(GLES_APIENTRY *DrawRangeElementsFunc)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices);
	typedef void			(GLES_APIENTRY *TexImage3DFunc)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void			(GLES_APIENTRY *TexSubImage3DFunc)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void			(GLES_APIENTRY *CopyTexSubImage3DFunc)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void			(GLES_APIENTRY *CompressedTexImage3DFunc)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void			(GLES_APIENTRY *CompressedTexSubImage3DFunc)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void			(GLES_APIENTRY *GenQueriesFunc)(GLsizei n, GLuint* ids);
	typedef void			(GLES_APIENTRY *DeleteQueriesFunc)(GLsizei n, const GLuint* ids);
	typedef GLboolean		(GLES_APIENTRY *UnmapBufferFunc)(GLenum target);
	typedef void			(GLES_APIENTRY *DrawBuffersFunc)(GLsizei n, const GLenum* bufs);
	typedef void			(GLES_APIENTRY *UniformMatrix2x3fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix3x2fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix2x4fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix4x2fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix3x4fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *UniformMatrix4x3fvFunc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *BlitFramebufferFunc)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	typedef void			(GLES_APIENTRY *RenderbufferStorageMultisampleFunc)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void			(GLES_APIENTRY *FramebufferTextureFunc)(GLenum target, GLenum attachment, GLuint texture, GLint level);
	typedef void			(GLES_APIENTRY *FramebufferTextureLayerFunc)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
	typedef GLvoid*			(GLES_APIENTRY *MapBufferRangeFunc)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
	typedef void			(GLES_APIENTRY *FlushMappedBufferRangeFunc)(GLenum target, GLintptr offset, GLsizeiptr length);
	typedef void			(GLES_APIENTRY *BindVertexArrayFunc)(GLuint array);
	typedef GLboolean		(GLES_APIENTRY *IsVertexArrayFunc)(GLuint array);
	typedef void			(GLES_APIENTRY *DeleteVertexArraysFunc)(GLsizei n, const GLuint* arrays);
	typedef void			(GLES_APIENTRY *GenVertexArraysFunc)(GLsizei n, GLuint* arrays);
	typedef void			(GLES_APIENTRY *GetIntegeri_vFunc)(GLenum target, GLuint index, GLint* data);
	typedef void			(GLES_APIENTRY *BeginTransformFeedbackFunc)(GLenum primitiveMode);
	typedef void			(GLES_APIENTRY *EndTransformFeedbackFunc)(void);
	typedef void			(GLES_APIENTRY *BindBufferRangeFunc)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	typedef void			(GLES_APIENTRY *BindBufferBaseFunc)(GLenum target, GLuint index, GLuint buffer);
	typedef void			(GLES_APIENTRY *TransformFeedbackVaryingsFunc)(GLuint program, GLsizei count, const GLchar* const* varyings, GLenum bufferMode);
	typedef void			(GLES_APIENTRY *GetTransformFeedbackVaryingFunc)(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name);
	typedef void			(GLES_APIENTRY *VertexAttribIPointerFunc)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
	typedef void			(GLES_APIENTRY *GetVertexAttribIivFunc)(GLuint index, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetVertexAttribIuivFunc)(GLuint index, GLenum pname, GLuint* params);
	typedef void			(GLES_APIENTRY *VertexAttribI4iFunc)(GLuint index, GLint x, GLint y, GLint z, GLint w);
	typedef void			(GLES_APIENTRY *VertexAttribI4uiFunc)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	typedef void			(GLES_APIENTRY *VertexAttribI4ivFunc)(GLuint index, const GLint* v);
	typedef void			(GLES_APIENTRY *VertexAttribI4uivFunc)(GLuint index, const GLuint* v);
	typedef void			(GLES_APIENTRY *GetUniformuivFunc)(GLuint program, GLint location, GLuint* params);
	typedef GLint			(GLES_APIENTRY *GetFragDataLocationFunc)(GLuint program, const GLchar *name);
	typedef const GLubyte*	(GLES_APIENTRY *GetStringiFunc)(GLenum name, GLuint index);
	typedef void			(GLES_APIENTRY *CopyBufferSubDataFunc)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	typedef void			(GLES_APIENTRY *GetUniformIndicesFunc)(GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames, GLuint* uniformIndices);
	typedef void			(GLES_APIENTRY *GetActiveUniformsivFunc)(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params);
	typedef GLuint			(GLES_APIENTRY *GetUniformBlockIndexFunc)(GLuint program, const GLchar* uniformBlockName);
	typedef void			(GLES_APIENTRY *GetActiveUniformBlockivFunc)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetActiveUniformBlockNameFunc)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName);
	typedef void			(GLES_APIENTRY *UniformBlockBindingFunc)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
	typedef void			(GLES_APIENTRY *DrawArraysInstancedFunc)(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
	typedef void			(GLES_APIENTRY *DrawElementsBaseVertexFunc)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, int basevertex);
	typedef void			(GLES_APIENTRY *DrawElementsInstancedFunc)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instanceCount);
	typedef void			(GLES_APIENTRY *DrawElementsInstancedBaseVertexFunc)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instanceCount, int basevertex);
	typedef void			(GLES_APIENTRY *DrawArraysIndirectFunc)(GLenum mode, const GLvoid* indirect);
	typedef GLsync			(GLES_APIENTRY *FenceSyncFunc)(GLenum condition, GLbitfield flags);
	typedef void			(GLES_APIENTRY *DeleteSyncFunc)(GLsync sync);
	typedef GLenum			(GLES_APIENTRY *ClientWaitSyncFunc)(GLsync sync, GLbitfield flags, GLuint64 timeout);
	typedef void			(GLES_APIENTRY *GenSamplersFunc)(GLsizei count, GLuint* samplers);
	typedef void			(GLES_APIENTRY *DeleteSamplersFunc)(GLsizei count, const GLuint* samplers);
	typedef void			(GLES_APIENTRY *BindSamplerFunc)(GLuint unit, GLuint sampler);
	typedef void			(GLES_APIENTRY *SamplerParameteriFunc)(GLuint sampler, GLenum pname, GLint param);
	typedef void			(GLES_APIENTRY *SamplerParameterivFunc)(GLuint sampler, GLenum pname, const GLint* param);
	typedef void			(GLES_APIENTRY *SamplerParameterfFunc)(GLuint sampler, GLenum pname, GLfloat param);
	typedef void			(GLES_APIENTRY *SamplerParameterfvFunc)(GLuint sampler, GLenum pname, const GLfloat* param);
	typedef void			(GLES_APIENTRY *GetSamplerParameterivFunc)(GLuint sampler, GLenum pname, GLint* params);
	typedef void			(GLES_APIENTRY *GetSamplerParameterfvFunc)(GLuint sampler, GLenum pname, GLfloat* params);
	typedef void			(GLES_APIENTRY *VertexAttribDivisorFunc)(GLuint index, GLuint divisor);
	typedef void			(GLES_APIENTRY *BindTransformFeedbackFunc)(GLenum target, GLuint id);
	typedef void			(GLES_APIENTRY *DeleteTransformFeedbacksFunc)(GLsizei n, const GLuint* ids);
	typedef void			(GLES_APIENTRY *GenTransformFeedbacksFunc)(GLsizei n, GLuint* ids);
	typedef GLboolean		(GLES_APIENTRY *IsTransformFeedbackFunc)(GLuint id);
	typedef void			(GLES_APIENTRY *PauseTransformFeedbackFunc)(void);
	typedef void			(GLES_APIENTRY *ResumeTransformFeedbackFunc)(void);
	typedef void			(GLES_APIENTRY *GetProgramBinaryFunc)(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary);
	typedef void			(GLES_APIENTRY *ProgramBinaryFunc)(GLuint program, GLenum binaryFormat, const GLvoid* binary, GLsizei length);
	typedef void			(GLES_APIENTRY *ProgramParameteriFunc)(GLuint program, GLenum pname, GLint value);
	typedef void			(GLES_APIENTRY *InvalidateFramebufferFunc)(GLenum target, GLsizei numAttachments, const GLenum* attachments);
	typedef void			(GLES_APIENTRY *InvalidateSubFramebufferFunc)(GLenum target, GLsizei numAttachments, const GLenum* attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void			(GLES_APIENTRY *TexStorage2DFunc)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void			(GLES_APIENTRY *TexStorage3DFunc)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	typedef void			(GLES_APIENTRY *GetInternalformativFunc)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params);

	// -- OpenGL 3.3 / GL_ARB_timer_query / GL_EXT_disjoint_timer_query --
	typedef void			(GLES_APIENTRY *GetQueryObjectui64vFunc) (GLuint id, GLenum pname, GLuint64* params);
	typedef void			(GLES_APIENTRY *QueryCounterFunc) (GLuint id, GLenum target);

	// -- OpenGL 4.4 / GL_ARB_clear_buffer_object
	typedef void			(GLES_APIENTRY *ClearBufferDataFunc)(GLenum target, GLenum internalformat, GLenum format, GLenum type, const GLvoid * data);
	typedef void			(GLES_APIENTRY *ClearBufferSubDataFunc)(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const GLvoid * data);

	// -- OpenGL 1.5 / GL_ARB_vertex_buffer_object / GL_OES_mapbuffer --
	typedef void*			(GLES_APIENTRY *MapBufferFunc)(GLenum target, GLenum access);

	// -- Desktop GL compatibility and core profile helpers --
	typedef void			(GLES_APIENTRY *DrawBufferFunc)(const GLenum buf);

	// -- GL_EXT_multisampled_render_to_texture / GL_IMG_multisampled_render_to_texture / GL_APPLE_framebuffer_multisample --
	typedef void			(GLES_APIENTRY *FramebufferTexture2DMultisampleEXTFunc)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

	// -- Extension only: GL_APPLE_framebuffer_multisample --
	typedef void			(GLES_APIENTRY *ResolveMultisampleFramebufferAPPLEFunc)(void);

	// -- OpenGL ES 3.1 / OpenGL 4.3 / ARB_compute_shader / ARB_shader_storage_buffer_object / ARB_shader_image_load_store --
	typedef void			(GLES_APIENTRY *DispatchComputeFunc)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
	typedef void			(GLES_APIENTRY *DispatchComputeIndirectFunc)(GLintptr indirect);
	typedef void			(GLES_APIENTRY *MemoryBarrierFunc)(GLbitfield barriers);
	typedef void			(GLES_APIENTRY *BindImageTextureFunc)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
	typedef void			(GLES_APIENTRY *TexStorage2DMultisampleFunc)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	typedef void			(GLES_APIENTRY *GetProgramInterfaceivFunc)(GLuint program, GLenum programInterface, const GLenum pname, GLint *params);
	typedef void			(GLES_APIENTRY *GetProgramResourceNameFunc)(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name);
	typedef void			(GLES_APIENTRY *GetProgramResourceivFunc)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);

	typedef void			(GLES_APIENTRY *ProgramUniform1fFunc)(GLuint program, GLint location, GLfloat x);
	typedef void			(GLES_APIENTRY *ProgramUniform1fvFunc)(GLuint program, GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *ProgramUniform1iFunc)(GLuint program, GLint location, GLint x);
	typedef void			(GLES_APIENTRY *ProgramUniform1ivFunc)(GLuint program, GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *ProgramUniform2fFunc)(GLuint program, GLint location, GLfloat x, GLfloat y);
	typedef void			(GLES_APIENTRY *ProgramUniform2fvFunc)(GLuint program, GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *ProgramUniform2iFunc)(GLuint program, GLint location, GLint x, GLint y);
	typedef void			(GLES_APIENTRY *ProgramUniform2ivFunc)(GLuint program, GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *ProgramUniform3fFunc)(GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z);
	typedef void			(GLES_APIENTRY *ProgramUniform3fvFunc)(GLuint program, GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *ProgramUniform3iFunc)(GLuint program, GLint location, GLint x, GLint y, GLint z);
	typedef void			(GLES_APIENTRY *ProgramUniform3ivFunc)(GLuint program, GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *ProgramUniform4fFunc)(GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void			(GLES_APIENTRY *ProgramUniform4fvFunc)(GLuint program, GLint location, GLsizei count, const GLfloat* v);
	typedef void			(GLES_APIENTRY *ProgramUniform4iFunc)(GLuint program, GLint location, GLint x, GLint y, GLint z, GLint w);
	typedef void			(GLES_APIENTRY *ProgramUniform4ivFunc)(GLuint program, GLint location, GLsizei count, const GLint* v);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix2fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix3fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix4fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix2x3fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix3x2fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix2x4fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix4x2fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix3x4fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniformMatrix4x3fvFunc)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	typedef void			(GLES_APIENTRY *ProgramUniform1uiFunc)(GLuint program, GLint location, GLuint v0);
	typedef void			(GLES_APIENTRY *ProgramUniform2uiFunc)(GLuint program, GLint location, GLuint v0, GLuint v1);
	typedef void			(GLES_APIENTRY *ProgramUniform3uiFunc)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
	typedef void			(GLES_APIENTRY *ProgramUniform4uiFunc)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	typedef void			(GLES_APIENTRY *ProgramUniform1uivFunc)(GLuint program, GLint location, GLsizei count, const GLuint* value);
	typedef void			(GLES_APIENTRY *ProgramUniform2uivFunc)(GLuint program, GLint location, GLsizei count, const GLuint* value);
	typedef void			(GLES_APIENTRY *ProgramUniform3uivFunc)(GLuint program, GLint location, GLsizei count, const GLuint* value);
	typedef void			(GLES_APIENTRY *ProgramUniform4uivFunc)(GLuint program, GLint location, GLsizei count, const GLuint* value);

	typedef void			(GLES_APIENTRY *CopyImageSubDataFunc)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

	// -- GL_KHR_debug / OpenGL 4.3 --
	typedef void			(GLES_APIENTRY *DebugCallbackFunc)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam);
	typedef void			(GLES_APIENTRY *DebugMessageControlFunc)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
	typedef void			(GLES_APIENTRY *DebugMessageCallbackFunc)(DebugCallbackFunc callback, const void* userParam);
	typedef void			(GLES_APIENTRY *DebugMessageInsertFunc)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf);
	typedef void			(GLES_APIENTRY *ObjectLabelFunc) (GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
	typedef void			(GLES_APIENTRY *GetObjectLabelFunc) (GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label);
	typedef void			(GLES_APIENTRY *ObjectPtrLabelFunc) (const void *ptr, GLsizei length, const GLchar *label);
	typedef void			(GLES_APIENTRY *GetObjectPtrLabelFunc) (const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label);
	typedef void			(GLES_APIENTRY *PushDebugGroupFunc)(GLenum source, GLuint id, GLsizei length, const char * message);
	typedef void			(GLES_APIENTRY *PopDebugGroupFunc)();

	// -- GL_EXT_debug_marker (ES extension only) --
	typedef void			(GLES_APIENTRY *PushGroupMarkerEXTFunc)(int len, const char* name);
	typedef void			(GLES_APIENTRY *PopGroupMarkerEXTFunc)();

	// -- GL_EXT_debug_label (ES extension only) --
	typedef void			(GLES_APIENTRY *LabelObjectEXTFunc) (GLenum type, GLuint object, GLsizei length, const GLchar *label);
	typedef void			(GLES_APIENTRY *GetObjectLabelEXTFunc) (GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label);

	// -- GL_ARB_vertex_attrib_64bit / OpenGL 4.1 --
	typedef void			(GLES_APIENTRY *VertexAttribLPointerFunc)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);

	// -- GL_KHR_blend_equation_advanced/GL_NV_blend_equation_advanced --
	typedef void			(GLES_APIENTRY *BlendBarrierFunc)();

	// -- OpenGL 1.0 --
	typedef void			(GLES_APIENTRY *PolygonModeFunc)(GLenum face, GLenum mode);
	typedef void			(GLES_APIENTRY *ClearDepthFunc)(GLdouble depth);

	// -- Tessellation --
	typedef void			(GLES_APIENTRY *PatchParameteriFunc) (GLenum pname, GLint value);
	typedef void			(GLES_APIENTRY *PatchParameterfvFunc) (GLenum pname, const GLfloat *values);

	// -- Sparse texture --
	typedef void (GLES_APIENTRY * TexPageCommitmentFunc) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);
	typedef void (GLES_APIENTRY * TexturePageCommitmentFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);

	// -- Buffer storage --
	typedef void (GLES_APIENTRY * BufferStorageFunc) (GLenum target, GLsizeiptr size, const void * data, GLbitfield flags);

	// -- GL_ARB_direct_state_access --
	typedef void (GLES_APIENTRY * CreateTexturesFunc) (GLenum target, GLsizei n, GLuint* textures);
	typedef void (GLES_APIENTRY * BindTextureUnitFunc) (GLuint unit, GLuint texture);
	typedef void (GLES_APIENTRY * TextureStorage1DFunc) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
	typedef void (GLES_APIENTRY * TextureStorage2DFunc) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void (GLES_APIENTRY * TextureStorage2DMultisampleFunc) (GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	typedef void (GLES_APIENTRY * TextureStorage3DFunc) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	typedef void (GLES_APIENTRY * TextureStorage3DMultisampleFunc) (GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
	typedef void (GLES_APIENTRY * TextureSubImage1DFunc) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
	typedef void (GLES_APIENTRY * TextureSubImage2DFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
	typedef void (GLES_APIENTRY * TextureSubImage3DFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	typedef void (GLES_APIENTRY * TextureBufferFunc) (GLuint texture, GLenum internalformat, GLuint buffer);
	typedef void (GLES_APIENTRY * TextureBufferRangeFunc) (GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	typedef void (GLES_APIENTRY * TextureParameteriFunc) (GLuint texture, GLenum pname, GLint param);
	typedef void (GLES_APIENTRY * TextureParameteriivFunc) (GLuint texture, GLenum pname, const GLint* params);
	typedef void (GLES_APIENTRY * TextureParameteriuivFunc) (GLuint texture, GLenum pname, const GLuint* params);
	typedef void (GLES_APIENTRY * TextureParameterfFunc) (GLuint texture, GLenum pname, GLfloat param);
	typedef void (GLES_APIENTRY * TextureParameterfvFunc) (GLuint texture, GLenum pname, const GLfloat* param);
	typedef void (GLES_APIENTRY * TextureParameteriFunc) (GLuint texture, GLenum pname, GLint param);
	typedef void (GLES_APIENTRY * TextureParameterivFunc) (GLuint texture, GLenum pname, const GLint* param);
	typedef void (GLES_APIENTRY * CompressedTextureSubImage1DFunc) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
	typedef void (GLES_APIENTRY * CompressedTextureSubImage2DFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
	typedef void (GLES_APIENTRY * CompressedTextureSubImage3DFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	typedef void (GLES_APIENTRY * CopyTextureSubImage1DFunc) (GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	typedef void (GLES_APIENTRY * CopyTextureSubImage2DFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (GLES_APIENTRY * CopyTextureSubImage3DFunc) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (GLES_APIENTRY * GenerateTextureMipmapFunc) (GLuint texture);
	typedef void (GLES_APIENTRY * GetCompressedTextureImageFunc) (GLuint texture, GLint level, GLsizei bufSize, void *pixels);
	typedef void (GLES_APIENTRY * GetTextureImageFunc) (GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	typedef void (GLES_APIENTRY * GetTextureLevelParameterfvFunc) (GLuint texture, GLint level, GLenum pname, GLfloat* params);
	typedef void (GLES_APIENTRY * GetTextureLevelParameterivFunc) (GLuint texture, GLint level, GLenum pname, GLint* params);
	typedef void (GLES_APIENTRY * GetTextureLevelParameteriivFunc) (GLuint texture, GLenum pname, GLint* params);
	typedef void (GLES_APIENTRY * GetTextureLevelParameteriuivFunc) (GLuint texture, GLenum pname, GLuint* params);
	typedef void (GLES_APIENTRY * GetTextureParameterfvFunc) (GLuint texture, GLenum pname, GLfloat* params);
	typedef void (GLES_APIENTRY * GetTextureParameterivFunc) (GLuint texture, GLenum pname, GLint* params);

	typedef void (GLES_APIENTRY * CreateFramebuffersFunc) (GLsizei n, GLuint* framebuffers);
	typedef void (GLES_APIENTRY * BlitNamedFramebufferFunc) (GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	typedef void (GLES_APIENTRY * ClearNamedFramebufferiFunc) (GLuint framebuffer, GLenum buffer, GLfloat depth, GLint stencil);
	typedef void (GLES_APIENTRY * ClearNamedFramebufferfvFunc) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value);
	typedef void (GLES_APIENTRY * ClearNamedFramebufferivFunc) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value);
	typedef void (GLES_APIENTRY * ClearNamedFramebufferuivFunc) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value);
	typedef GLenum (GLES_APIENTRY * CheckNamedFramebufferStatusFunc) (GLuint framebuffer, GLenum target);
	typedef void (GLES_APIENTRY * GetNamedFramebufferParameterivFunc) (GLuint framebuffer, GLenum pname, GLint* param);
	typedef void (GLES_APIENTRY * GetNamedFramebufferAttachmentParameterivFunc) (GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params);

	typedef void (GLES_APIENTRY * InvalidateNamedFramebufferDataFunc) (GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments);
	typedef void (GLES_APIENTRY * InvalidateNamedFramebufferSubDataFunc) (GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (GLES_APIENTRY * NamedFramebufferDrawBufferFunc) (GLuint framebuffer, GLenum mode);
	typedef void (GLES_APIENTRY * NamedFramebufferDrawBuffersFunc) (GLuint framebuffer, GLsizei n, const GLenum* bufs);
	typedef void (GLES_APIENTRY * NamedFramebufferParameteriFunc) (GLuint framebuffer, GLenum pname, GLint param);
	typedef void (GLES_APIENTRY * NamedFramebufferReadBufferFunc) (GLuint framebuffer, GLenum mode);
	typedef void (GLES_APIENTRY * NamedFramebufferRenderbufferFunc) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	typedef void (GLES_APIENTRY * NamedFramebufferTextureFunc) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
	typedef void (GLES_APIENTRY * NamedFramebufferTextureLayerFunc) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);

	typedef void (GLES_APIENTRY * CreateBuffersFunc) (GLsizei n, GLuint* buffers);
	typedef void (GLES_APIENTRY * ClearNamedBufferDataFunc) (GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
	typedef void (GLES_APIENTRY * ClearNamedBufferSubDataFunc) (GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
	typedef void (GLES_APIENTRY * NamedBufferStorageFunc) (GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
	typedef void (GLES_APIENTRY * NamedBufferDataFunc) (GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
	typedef void (GLES_APIENTRY * NamedBufferSubDataFunc) (GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
	typedef void (GLES_APIENTRY * CopyNamedBufferSubDataFunc) (GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	typedef void (GLES_APIENTRY * FlushMappedNamedBufferRangeFunc) (GLuint buffer, GLintptr offset, GLsizeiptr length);
	typedef void * (GLES_APIENTRY * MapNamedBufferFunc) (GLuint buffer, GLenum access);
	typedef void * (GLES_APIENTRY * MapNamedBufferRangeFunc) (GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
	typedef GLboolean (GLES_APIENTRY * UnmapNamedBufferFunc) (GLuint buffer);
	typedef void (GLES_APIENTRY * GetNamedBufferSubDataFunc) (GLuint buffer, GLintptr offset, GLsizeiptr size, void *data);
	typedef void (GLES_APIENTRY * GetNamedBufferParameteri64vFunc) (GLuint buffer, GLenum pname, GLint64* params);
	typedef void (GLES_APIENTRY * GetNamedBufferParameterivFunc) (GLuint buffer, GLenum pname, GLint* params);
	typedef void (GLES_APIENTRY * GetNamedBufferPointervFunc) (GLuint buffer, GLenum pname, void** params);
	typedef void (GLES_APIENTRY * NamedBufferStorageFunc) (GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);

	typedef void (GLES_APIENTRY * CreateRenderbuffersFunc) (GLsizei n, GLuint* renderbuffers);
	typedef void (GLES_APIENTRY * NamedRenderbufferStorageFunc) (GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void (GLES_APIENTRY * NamedRenderbufferStorageMultisampleFunc) (GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void (GLES_APIENTRY * GetNamedRenderbufferParameterivFunc) (GLuint renderbuffer, GLenum pname, GLint* params);

	typedef void (GLES_APIENTRY * CreateProgramPipelinesFunc) (GLsizei n, GLuint* pipelines);

	typedef void (GLES_APIENTRY * CreateSamplersFunc) (GLsizei n, GLuint* samplers);

	typedef void (GLES_APIENTRY * CreateTransformFeedbacksFunc) (GLsizei n, GLuint* ids);
	typedef void (GLES_APIENTRY * TransformFeedbackBufferBaseFunc) (GLuint xfb, GLuint index, GLuint buffer);
	typedef void (GLES_APIENTRY * TransformFeedbackBufferRangeFunc) (GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	typedef void (GLES_APIENTRY * GetTransformFeedbacki64_vFunc) (GLuint xfb, GLenum pname, GLuint index, GLint64* param);
	typedef void (GLES_APIENTRY * GetTransformFeedbacki_vFunc) (GLuint xfb, GLenum pname, GLuint index, GLint* param);
	typedef void (GLES_APIENTRY * GetTransformFeedbackivFunc) (GLuint xfb, GLenum pname, GLint* param);

	typedef void (GLES_APIENTRY * CreateVertexArraysFunc) (GLsizei n, GLuint* arrays);
	typedef void (GLES_APIENTRY * DisableVertexArrayAttribFunc) (GLuint vaobj, GLuint index);
	typedef void (GLES_APIENTRY * EnableVertexArrayAttribFunc) (GLuint vaobj, GLuint index);
	typedef void (GLES_APIENTRY * GetVertexArrayIndexed64ivFunc) (GLuint vaobj, GLuint index, GLenum pname, GLint64* param);
	typedef void (GLES_APIENTRY * GetVertexArrayIndexedivFunc) (GLuint vaobj, GLuint index, GLenum pname, GLint* param);
	typedef void (GLES_APIENTRY * GetVertexArrayivFunc) (GLuint vaobj, GLenum pname, GLint* param);

	typedef void (GLES_APIENTRY * VertexArrayAttribBindingFunc) (GLuint vaobj, GLuint attribindex, GLuint bindingindex);
	typedef void (GLES_APIENTRY * VertexArrayAttribFormatFunc) (GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
	typedef void (GLES_APIENTRY * VertexArrayAttribIFormatFunc) (GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	typedef void (GLES_APIENTRY * VertexArrayAttribLFormatFunc) (GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	typedef void (GLES_APIENTRY * VertexArrayBindingDivisorFunc) (GLuint vaobj, GLuint bindingindex, GLuint divisor);
	typedef void (GLES_APIENTRY * VertexArrayElementBufferFunc) (GLuint vaobj, GLuint buffer);
	typedef void (GLES_APIENTRY * VertexArrayVertexBufferFunc) (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	typedef void (GLES_APIENTRY * VertexArrayVertexBuffersFunc) (GLuint vaobj, GLuint first, GLsizei count, const GLuint* buffers, const GLintptr *offsets, const GLsizei *strides);
}//namespace gl

struct ApiFuncGLES
{
public:
	ApiFuncGLES()
	{
		// Zero-initialize everything, this is POD struct.
		memset(this, 0, sizeof(ApiFuncGLES));
	}

	gl::BindAttribLocationFunc						glBindAttribLocation;
	gl::BlendEquationFunc							glBlendEquation;
	gl::BlendEquationSeparateFunc					glBlendEquationSeparate;
	gl::BlendFuncSeparateFunc						glBlendFuncSeparate;
	gl::ColorMaskFunc								glColorMask;
	gl::DepthFuncFunc								glDepthFunc;
	gl::DepthMaskFunc								glDepthMask;
	gl::FinishFunc									glFinish;
	gl::FlushFunc									glFlush;
	gl::FramebufferRenderbufferFunc					glFramebufferRenderbuffer;
	gl::FramebufferTexture2DFunc					glFramebufferTexture2D;
	gl::FramebufferTextureLayerFunc					glFramebufferTextureLayer;
	gl::FramebufferTextureFunc						glFramebufferTexture;
	gl::FrontFaceFunc								glFrontFace;
	gl::GetActiveAttribFunc							glGetActiveAttrib;
	gl::GetActiveUniformFunc						glGetActiveUniform;
	gl::GetAttribLocationFunc						glGetAttribLocation;
	gl::GetErrorFunc								glGetError;
	gl::GetIntegervFunc								glGetIntegerv;
	gl::GetProgramivFunc							glGetProgramiv;
	gl::GetTexParameterivFunc						glGetTexParameteriv;
	gl::GetUniformivFunc							glGetUniformiv;
	gl::GetUniformLocationFunc						glGetUniformLocation;
	gl::GetVertexAttribfvFunc						glGetVertexAttribfv;
	gl::GetVertexAttribivFunc						glGetVertexAttribiv;
	gl::GetVertexAttribPointervFunc					glGetVertexAttribPointerv;
	gl::PolygonOffsetFunc							glPolygonOffset;
	gl::ReadPixelsFunc								glReadPixels;
	gl::ScissorFunc									glScissor;
	gl::Uniform1fvFunc								glUniform1fv;
	gl::Uniform1iFunc								glUniform1i;
	gl::Uniform1ivFunc								glUniform1iv;
	gl::Uniform2fvFunc								glUniform2fv;
	gl::Uniform2ivFunc								glUniform2iv;
	gl::Uniform3fvFunc								glUniform3fv;
	gl::Uniform3ivFunc								glUniform3iv;
	gl::Uniform4fvFunc								glUniform4fv;
	gl::Uniform4ivFunc								glUniform4iv;
	gl::UniformMatrix3fvFunc						glUniformMatrix3fv;
	gl::UniformMatrix4fvFunc						glUniformMatrix4fv;
	gl::VertexAttrib4fFunc							glVertexAttrib4f;
	gl::ViewportFunc								glViewport;

	gl::TexImage2DFunc								glTexImage2D;
	gl::TexParameteriFunc							glTexParameteri;
	gl::CompressedTexImage2DFunc					glCompressedTexImage2D;
	gl::CopyTexImage2DFunc							glCopyTexImage2D;
	gl::CopyTexSubImage2DFunc						glCopyTexSubImage2D;

	gl::ReadBufferFunc								glReadBuffer;
	gl::TransformFeedbackVaryingsFunc				glTransformFeedbackVaryings;
	gl::GetActiveUniformsivFunc						glGetActiveUniformsiv;
	gl::GetUniformBlockIndexFunc					glGetUniformBlockIndex;
	gl::GetActiveUniformBlockivFunc					glGetActiveUniformBlockiv;
	gl::GetActiveUniformBlockNameFunc				glGetActiveUniformBlockName;
	gl::UniformBlockBindingFunc						glUniformBlockBinding;
	gl::GetProgramBinaryFunc						glGetProgramBinary;
	gl::ProgramBinaryFunc							glProgramBinary;
	gl::ProgramParameteriFunc						glProgramParameteri;
	gl::InvalidateFramebufferFunc					glInvalidateFramebuffer;

	gl::DispatchComputeFunc							glDispatchCompute;
	gl::DispatchComputeIndirectFunc					glDispatchComputeIndirect;
	gl::MemoryBarrierFunc							glMemoryBarrier;
	gl::BindImageTextureFunc						glBindImageTexture;
	gl::GetProgramInterfaceivFunc					glGetProgramInterfaceiv;
	gl::GetProgramResourceNameFunc					glGetProgramResourceName;
	gl::GetProgramResourceivFunc					glGetProgramResourceiv;

	gl::ProgramUniform1fvFunc						glProgramUniform1fv;
	gl::ProgramUniform1ivFunc						glProgramUniform1iv;
	gl::ProgramUniform2fvFunc						glProgramUniform2fv;
	gl::ProgramUniform2ivFunc						glProgramUniform2iv;
	gl::ProgramUniform3fvFunc						glProgramUniform3fv;
	gl::ProgramUniform3ivFunc						glProgramUniform3iv;
	gl::ProgramUniform4fvFunc						glProgramUniform4fv;
	gl::ProgramUniform4ivFunc						glProgramUniform4iv;
	gl::ProgramUniformMatrix2fvFunc					glProgramUniformMatrix2fv;
	gl::ProgramUniformMatrix3fvFunc					glProgramUniformMatrix3fv;
	gl::ProgramUniformMatrix4fvFunc					glProgramUniformMatrix4fv;
	gl::ProgramUniformMatrix2x3fvFunc				glProgramUniformMatrix2x3fv;
	gl::ProgramUniformMatrix3x2fvFunc				glProgramUniformMatrix3x2fv;
	gl::ProgramUniformMatrix2x4fvFunc				glProgramUniformMatrix2x4fv;
	gl::ProgramUniformMatrix4x2fvFunc				glProgramUniformMatrix4x2fv;
	gl::ProgramUniformMatrix3x4fvFunc				glProgramUniformMatrix3x4fv;
	gl::ProgramUniformMatrix4x3fvFunc				glProgramUniformMatrix4x3fv;
	gl::ProgramUniform1uivFunc						glProgramUniform1uiv;
	gl::ProgramUniform2uivFunc						glProgramUniform2uiv;
	gl::ProgramUniform3uivFunc						glProgramUniform3uiv;
	gl::ProgramUniform4uivFunc						glProgramUniform4uiv;

	gl::FenceSyncFunc								glFenceSync;
	gl::ClientWaitSyncFunc							glClientWaitSync;
	gl::DeleteSyncFunc								glDeleteSync;

	gl::GetShaderPrecisionFormatFunc				glGetShaderPrecisionFormat;

	// iOS
	gl::FramebufferTexture2DMultisampleEXTFunc		glFramebufferTexture2DMultisampleEXT;
	gl::ResolveMultisampleFramebufferAPPLEFunc		glResolveMultisampleFramebufferAPPLE;

	// GL_KHR_blend_equation_advanced/GL_NV_blend_equation_advanced
	gl::BlendBarrierFunc							glBlendBarrier;

public:
	// -- OpenGL 4.3 or KHR_debug (GL & ES) --
	gl::ObjectLabelFunc							glObjectLabel;
	gl::GetObjectLabelFunc						glGetObjectLabel;
	gl::PushDebugGroupFunc						glPushDebugGroup;
	gl::PopDebugGroupFunc						glPopDebugGroup;

	// -- EXT_debug_label (ES) --
	gl::LabelObjectEXTFunc						glLabelObjectEXT;
	gl::GetObjectLabelEXTFunc					glGetObjectLabelEXT;

	// -- EXT_debug_marker (ES) --
	gl::PushGroupMarkerEXTFunc					glPushGroupMarkerEXT;
	gl::PopGroupMarkerEXTFunc					glPopGroupMarkerEXT;

	// -- Debug output, OpenGL 4.3 or KHR_debug (GL & ES) --
	gl::DebugMessageControlFunc					glDebugMessageControl;
	gl::DebugMessageCallbackFunc				glDebugMessageCallback;
	gl::DebugMessageInsertFunc					glDebugMessageInsert;

	// -- Texture image data copy (GL 4.3 / ES 3.2)
	gl::CopyImageSubDataFunc					glCopyImageSubData;

protected:
	// -- Draw --
	gl::DrawArraysFunc							glDrawArrays;
	gl::DrawArraysInstancedFunc					glDrawArraysInstanced;
	gl::DrawArraysIndirectFunc					glDrawArraysIndirect;
	gl::DrawElementsFunc						glDrawElements;
	gl::DrawElementsBaseVertexFunc				glDrawElementsBaseVertex;
	gl::DrawElementsInstancedFunc				glDrawElementsInstanced;
	gl::DrawElementsInstancedBaseVertexFunc		glDrawElementsInstancedBaseVertex;
	gl::ClearFunc								glClear;
	gl::ClearColorFunc							glClearColor;
	gl::ClearDepthfFunc							glClearDepthf;
	gl::ClearStencilFunc						glClearStencil;

	// -- Shader --
	gl::CreateShaderFunc						glCreateShader;
	gl::ShaderSourceFunc						glShaderSource;
	gl::CompileShaderFunc						glCompileShader;
	gl::DeleteShaderFunc						glDeleteShader;
	gl::GetShaderivFunc							glGetShaderiv;
	gl::GetShaderInfoLogFunc					glGetShaderInfoLog;
	gl::GetShaderSourceFunc						glGetShaderSource;

	// -- Program --
	gl::CreateProgramFunc						glCreateProgram;
	gl::DeleteProgramFunc						glDeleteProgram;
	gl::DetachShaderFunc						glDetachShader;
	gl::UseProgramFunc							glUseProgram;
	gl::LinkProgramFunc							glLinkProgram;
	gl::AttachShaderFunc						glAttachShader;
	//gl::GetProgramivFunc						glGetProgramiv; // TODO, bring here
	gl::GetProgramInfoLogFunc					glGetProgramInfoLog;
	gl::ValidateProgramFunc						glValidateProgram;

	gl::CreateProgramPipelinesFunc						glCreateProgramPipelines;

	// -- Framebuffer --
	gl::GenFramebuffersFunc								glGenFramebuffers;
	gl::DeleteFramebuffersFunc							glDeleteFramebuffers;
	gl::BindFramebufferFunc								glBindFramebuffer;
	gl::BlitFramebufferFunc								glBlitFramebuffer;
	gl::CheckFramebufferStatusFunc						glCheckFramebufferStatus;
	gl::GetFramebufferAttachmentParameterivFunc			glGetFramebufferAttachmentParameteriv;
	gl::DrawBufferFunc									glDrawBuffer; // OpenGL 1.1
	gl::DrawBuffersFunc									glDrawBuffers;
	gl::ClearDepthFunc									glClearDepth;

	gl::BlitNamedFramebufferFunc						glBlitNamedFramebuffer;
	gl::ClearNamedFramebufferiFunc						glClearNamedFramebufferi;
	gl::ClearNamedFramebufferfvFunc						glClearNamedFramebufferfv;
	gl::ClearNamedFramebufferivFunc						glClearNamedFramebufferiv;
	gl::ClearNamedFramebufferuivFunc					glClearNamedFramebufferuiv;
	gl::CheckNamedFramebufferStatusFunc					glCheckNamedFramebufferStatus;
	gl::GetNamedFramebufferParameterivFunc				glGetNamedFramebufferParameteriv;
	gl::GetNamedFramebufferAttachmentParameterivFunc	glGetNamedFramebufferAttachmentParameteriv;
	gl::InvalidateNamedFramebufferDataFunc				glInvalidateNamedFramebufferData;
	gl::InvalidateNamedFramebufferSubDataFunc			glInvalidateNamedFramebufferSubData;
	gl::NamedFramebufferDrawBufferFunc					glNamedFramebufferDrawBuffer;
	gl::NamedFramebufferDrawBuffersFunc					glNamedFramebufferDrawBuffers;
	gl::NamedFramebufferParameteriFunc					glNamedFramebufferParameteri;
	gl::NamedFramebufferReadBufferFunc					glNamedFramebufferReadBuffer;
	gl::NamedFramebufferRenderbufferFunc				glNamedFramebufferRenderbuffer;
	gl::NamedFramebufferTextureFunc						glNamedFramebufferTexture;
	gl::NamedFramebufferTextureLayerFunc				glNamedFramebufferTextureLayer;

	// -- Texture --
	gl::TexImage3DFunc									glTexImage3D;
	gl::TexImage2DMultisampleFunc						glTexImage2DMultisample;
	gl::TexSubImage2DFunc								glTexSubImage2D;
	gl::TexSubImage3DFunc								glTexSubImage3D;
	gl::TexStorage2DFunc								glTexStorage2D;
	gl::TexStorage2DMultisampleFunc						glTexStorage2DMultisample;
	gl::TexStorage3DFunc								glTexStorage3D;
	gl::CompressedTexImage3DFunc						glCompressedTexImage3D;
	gl::CompressedTexSubImage2DFunc						glCompressedTexSubImage2D;
	gl::CompressedTexSubImage3DFunc						glCompressedTexSubImage3D;
	gl::TexParameterivFunc								glTexParameteriv;
	gl::TexParameterfFunc								glTexParameterf;
	gl::GenerateMipmapFunc								glGenerateMipmap;

	// -- Renderbuffer --
	gl::GenRenderbuffersFunc							glGenRenderbuffers;
	gl::DeleteRenderbuffersFunc							glDeleteRenderbuffers;
	gl::BindRenderbufferFunc							glBindRenderbuffer;
	gl::RenderbufferStorageFunc							glRenderbufferStorage;
	gl::RenderbufferStorageMultisampleFunc				glRenderbufferStorageMultisample;

	gl::CreateRenderbuffersFunc							glCreateRenderbuffers;
	gl::NamedRenderbufferStorageFunc					glNamedRenderbufferStorage;
	gl::NamedRenderbufferStorageMultisampleFunc			glNamedRenderbufferStorageMultisample;
	gl::GetNamedRenderbufferParameterivFunc				glGetNamedRenderbufferParameteriv;

	// -- Buffer --
	gl::GenBuffersFunc									glGenBuffers;
	gl::DeleteBuffersFunc								glDeleteBuffers;
	gl::BindBufferFunc									glBindBuffer;
	gl::BindBufferRangeFunc								glBindBufferRange;
	gl::BindBufferBaseFunc								glBindBufferBase;
	gl::BufferDataFunc									glBufferData;
	gl::BufferSubDataFunc								glBufferSubData;
	gl::MapBufferFunc									glMapBuffer;
	gl::MapBufferRangeFunc								glMapBufferRange;
	gl::UnmapBufferFunc									glUnmapBuffer;
	gl::FlushMappedBufferRangeFunc						glFlushMappedBufferRange;
	gl::GetIntegeri_vFunc								glGetIntegeri_v;
	gl::CopyBufferSubDataFunc							glCopyBufferSubData;
	gl::ClearBufferDataFunc								glClearBufferData;
	gl::ClearBufferSubDataFunc							glClearBufferSubData;
	gl::BufferStorageFunc								glBufferStorage;

	gl::CreateBuffersFunc								glCreateBuffers;
	gl::ClearNamedBufferDataFunc						glClearNamedBufferData;
	gl::ClearNamedBufferSubDataFunc						glClearNamedBufferSubData;
	gl::NamedBufferStorageFunc							glNamedBufferStorage;
	gl::NamedBufferDataFunc								glNamedBufferData;
	gl::NamedBufferSubDataFunc							glNamedBufferSubData;
	gl::CopyNamedBufferSubDataFunc						glCopyNamedBufferSubData;
	gl::FlushMappedNamedBufferRangeFunc					glFlushMappedNamedBufferRange;
	gl::MapNamedBufferFunc								glMapNamedBuffer;
	gl::MapNamedBufferRangeFunc							glMapNamedBufferRange;
	gl::UnmapNamedBufferFunc							glUnmapNamedBuffer;
	gl::GetNamedBufferSubDataFunc						glGetNamedBufferSubData;
	gl::GetNamedBufferParameteri64vFunc					glGetNamedBufferParameteri64v;
	gl::GetNamedBufferParameterivFunc					glGetNamedBufferParameteriv;
	gl::GetNamedBufferPointervFunc						glGetNamedBufferPointerv;

	// -- Vertex Array --
	gl::BindVertexArrayFunc							glBindVertexArray;
	gl::IsVertexArrayFunc							glIsVertexArray;
	gl::VertexAttribPointerFunc						glVertexAttribPointer;
	gl::VertexAttribIPointerFunc					glVertexAttribIPointer;
	gl::VertexAttribLPointerFunc					glVertexAttribLPointer;
	gl::DisableVertexAttribArrayFunc				glDisableVertexAttribArray;
	gl::EnableVertexAttribArrayFunc					glEnableVertexAttribArray;
	gl::DeleteVertexArraysFunc						glDeleteVertexArrays;
	gl::GenVertexArraysFunc							glGenVertexArrays;

	gl::CreateVertexArraysFunc						glCreateVertexArrays;
	gl::DisableVertexArrayAttribFunc				glDisableVertexArrayAttrib;
	gl::EnableVertexArrayAttribFunc					glEnableVertexArrayAttrib;
	gl::GetVertexArrayIndexed64ivFunc				glGetVertexArrayIndexed64iv;
	gl::GetVertexArrayIndexedivFunc					glGetVertexArrayIndexediv;
	gl::GetVertexArrayivFunc						glGetVertexArrayiv;
	gl::VertexArrayAttribBindingFunc				glVertexArrayAttribBinding;
	gl::VertexArrayAttribFormatFunc					glVertexArrayAttribFormat;
	gl::VertexArrayAttribIFormatFunc				glVertexArrayAttribIFormat;
	gl::VertexArrayAttribLFormatFunc				glVertexArrayAttribLFormat;
	gl::VertexArrayBindingDivisorFunc				glVertexArrayBindingDivisor;
	gl::VertexArrayElementBufferFunc				glVertexArrayElementBuffer;
	gl::VertexArrayVertexBufferFunc					glVertexArrayVertexBuffer;
	gl::VertexArrayVertexBuffersFunc				glVertexArrayVertexBuffers;

	// -- Transform feedback --
	gl::BindTransformFeedbackFunc					glBindTransformFeedback;
	gl::DeleteTransformFeedbacksFunc				glDeleteTransformFeedbacks;
	gl::GenTransformFeedbacksFunc					glGenTransformFeedbacks;
	gl::BeginTransformFeedbackFunc					glBeginTransformFeedback;
	gl::EndTransformFeedbackFunc					glEndTransformFeedback;

	gl::CreateTransformFeedbacksFunc				glCreateTransformFeedbacks;
	gl::TransformFeedbackBufferBaseFunc				glTransformFeedbackBufferBase;
	gl::TransformFeedbackBufferRangeFunc			glTransformFeedbackBufferRange;
	gl::GetTransformFeedbacki64_vFunc				glGetTransformFeedbacki64_v;
	gl::GetTransformFeedbacki_vFunc					glGetTransformFeedbacki_v;
	gl::GetTransformFeedbackivFunc					glGetTransformFeedbackiv;

	// -- Rasterizer --
	gl::CullFaceFunc							glCullFace;

	// -- Tessellation --
	gl::PatchParameteriFunc						glPatchParameteri;
	gl::PatchParameterfvFunc					glPatchParameterfv;

	// -- Stencil --
	gl::StencilFuncFunc							glStencilFunc;
	gl::StencilFuncSeparateFunc					glStencilFuncSeparate;
	gl::StencilMaskFunc							glStencilMask;
	gl::StencilOpFunc							glStencilOp;
	gl::StencilOpSeparateFunc					glStencilOpSeparate;

	// -- Enable / Disable --
	gl::IsEnabledFunc							glIsEnabled;
	gl::EnableFunc								glEnable;
	gl::DisableFunc								glDisable;

	// -- Misc --
	gl::PixelStoreiFunc							glPixelStorei;
	gl::PolygonModeFunc							glPolygonMode;

	gl::GenQueriesFunc							glGenQueries;			// GL 3.0 / ES 3.0 / GL_NV_timer_query / GL_EXT_disjoint_timer_query
	gl::DeleteQueriesFunc						glDeleteQueries;		// GL 3.0 / ES 3.0 / GL_NV_timer_query / GL_EXT_disjoint_timer_query
	gl::GetQueryObjectui64vFunc					glGetQueryObjectui64v;	// GL 3.3 / GL_ARB_timer_query / GL_NV_timer_query / GL_EXT_disjoint_timer_query
	gl::QueryCounterFunc						glQueryCounter;			// GL 3.3 / GL_ARB_timer_query / GL_NV_timer_query / GL_EXT_disjoint_timer_query

	// -- Texture --
public:
	gl::GenTexturesFunc							glGenTextures;

protected:
	gl::ActiveTextureFunc						glActiveTexture;
	gl::BindTextureFunc							glBindTexture;
	gl::DeleteTexturesFunc						glDeleteTextures;
	gl::TexPageCommitmentFunc					glTexPageCommitment;

	gl::CreateTexturesFunc						glCreateTextures;
	gl::BindTextureUnitFunc						glBindTextureUnit;
	gl::TextureStorage1DFunc					glTextureStorage1D;
	gl::TextureStorage2DFunc					glTextureStorage2D;
	gl::TextureStorage2DMultisampleFunc			glTextureStorage2DMultisample;
	gl::TextureStorage3DFunc					glTextureStorage3D;
	gl::TextureStorage3DMultisampleFunc			glTextureStorage3DMultisample;
	gl::TextureSubImage1DFunc					glTextureSubImage1D;
	gl::TextureSubImage2DFunc					glTextureSubImage2D;
	gl::TextureSubImage3DFunc					glTextureSubImage3D;
	gl::TexturePageCommitmentFunc				glTexturePageCommitment;
	gl::TextureBufferFunc						glTextureBuffer;
	gl::TextureBufferRangeFunc					glTextureBufferRange;
	gl::TextureParameteriFunc					glTextureParameteri;
	gl::TextureParameterivFunc					glTextureParameteriv;
	gl::TextureParameteriivFunc					glTextureParameteriiv;
	gl::TextureParameteriuivFunc				glTextureParameteriuiv;
	gl::TextureParameterfFunc					glTextureParameterf;
	gl::TextureParameterfvFunc					glTextureParameterfv;
	gl::CompressedTextureSubImage1DFunc			glCompressedTextureSubImage1D;
	gl::CompressedTextureSubImage2DFunc			glCompressedTextureSubImage2D;
	gl::CompressedTextureSubImage3DFunc			glCompressedTextureSubImage3D;
	gl::CopyTextureSubImage1DFunc				glCopyTextureSubImage1D;
	gl::CopyTextureSubImage2DFunc				glCopyTextureSubImage2D;
	gl::CopyTextureSubImage3DFunc				glCopyTextureSubImage3D;
	gl::GenerateTextureMipmapFunc				glGenerateTextureMipmap;
	gl::GetCompressedTextureImageFunc			glGetCompressedTextureImage;
	gl::GetTextureImageFunc						glGetTextureImage;
	gl::GetTextureLevelParameterfvFunc			glGetTextureLevelParameterfv;
	gl::GetTextureLevelParameterivFunc			glGetTextureLevelParameteriv;
	gl::GetTextureLevelParameteriivFunc			glGetTextureLevelParameteriiv;
	gl::GetTextureLevelParameteriuivFunc		glGetTextureLevelParameteriuiv;
	gl::GetTextureParameterfvFunc				glGetTextureParameterfv;
	gl::GetTextureParameterivFunc				glGetTextureParameteriv;

	// -- Sampler --
	gl::GenSamplersFunc							glGenSamplers;
	gl::DeleteSamplersFunc						glDeleteSamplers;
	gl::BindSamplerFunc							glBindSampler;
	gl::SamplerParameteriFunc					glSamplerParameteri;

	gl::CreateSamplersFunc						glCreateSamplers;

	// -- All initialization time code --
	gl::GetStringiFunc							glGetStringi;
	gl::GetStringFunc							glGetString;
	gl::GetInternalformativFunc					glGetInternalformativ;
};
