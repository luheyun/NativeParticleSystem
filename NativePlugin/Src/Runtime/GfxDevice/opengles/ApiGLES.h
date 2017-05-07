// This file contains a cross platform interface for OpenGL and OpenGL ES

#pragma once

#if !GFX_SUPPORTS_OPENGL_UNIFIED
	#error "Should not include OpenGL/ES on this platform"
#endif

#include "Runtime/Utilities/fixed_array.h"
#include "Runtime/Utilities/NonCopyable.h"
#include "Runtime/Math/Color.h"
#include "Runtime/Threads/Thread.h"
#include "Runtime/GfxDevice/GfxDeviceTypes.h"
#include "ApiEnumGLES.h"
#include "ApiFuncGLES.h"
#include "HandleGLES.h"

#define USE_GL_REDUNDANT_STATES_CACHING 1
#define USE_GL_FACTORIZED_TEX_UPLOAD_API 0

#undef NO_ERROR

#define CAN_HAVE_DIRECT_STATE_ACCESS 0//UNITY_WIN

class TranslateGLES;
struct GfxStencilState;

struct AttachmentDescGLES
{
	GLenum type;
	GLenum target;
	GLuint name;
	GLint level;
	GLsizei samples;
};

struct FramebufferInfoGLES
{
	GLint redBits;
	GLint greenBits;
	GLint blueBits;
	GLint alphaBits;
	GLint depthBits;
	GLint stencilBits;
	GLint samples;
	GLint sampleBuffers;
	GLint coverageSamples;
	GLint coverageBuffers;
};

// This is a simple counter type for tracking order of resource accesses that need memory barriers.
typedef unsigned long long BarrierTime;

class GfxContextGLES;

class ApiGLES : public ApiFuncGLES, private NonCopyable
{
	TranslateGLES* m_Translate;

public:
	static const GLuint kInvalidValue;

	ApiGLES();
	~ApiGLES();

	void Init(GfxDeviceLevelGL &deviceLevel);

	// -- Draw API --

	void Dispatch(UInt32 threadGroupsX, UInt32 threadGroupsY, UInt32 threadGroupsZ);

	void DispatchIndirect(UInt32 indirect);

	void DrawArrays(GfxPrimitiveType topology, UInt32 firstVertex, UInt32 vertexCount, UInt32 instanceCount);

	void DrawElements(GfxPrimitiveType topology, const void * indicesOrOffset, UInt32 indexCount, UInt32 baseVertex, UInt32 instanceCount);

	// Draw arrays writing transformed vertices in a bound transform feedback buffer
	void DrawCapture(GfxPrimitiveType topology, UInt32 VertexCount);

	// Rendering using draw parameters stored in OpenGL buffer memory
	void DrawIndirect(GfxPrimitiveType topology, UInt32 bufferOffset);

	// Clearing framebuffers
	void Clear(GLbitfield flags, const ColorRGBAf& color = ColorRGBAf(0, 0, 0, 1), bool onlyColorAlpha = false, float depth = 0.0f, int stencil = 0);

	// -- Shader API --

	// Create an OpenGL shader object and launch the shader compilation
	// We don't way for the result to allow drivers to multi thread the shader compilation
	GLuint CreateShader(gl::ShaderStage stage, const char* source);

	// Check the result of the shader compilation. Return true for success
	// Delete the shader object if it failed and set 'shader' to 0
	bool CheckShader(GLuint & shader, bool debug = false);

	// Delete a shader object and set to 0 its name
	void DeleteShader(GLuint & shader);

	// -- Program API --

	void BindProgram(GLuint program, bool hasTessStages = false);

	GLuint GetProgramBinding() const{return m_CurrentProgramBinding;}

	// When changing a binding, a layout or attaching a shader, the program needs to be linked again to take into account these changes.
	void LinkProgram(GLuint program);

	// Check the result of the program compilation. Return true for success
	// Delete the program object if it failed and set 'shader' to 0
	bool CheckProgram(GLuint & program);

	// Create a program object
	GLuint CreateProgram();

	GLuint CreateGraphicsProgram(GLuint vertexShader, GLuint controlShader, GLuint evaluationShader, GLuint geomtryShader, GLuint fragmentShader);

	GLuint CreateComputeProgram(GLuint computeShader);

	// Delete a program object and set to 0 its name
	void DeleteProgram(GLuint & program);

	FramebufferInfoGLES GetFramebufferInfo() const;

	// -- Renderbuffer API --

	GLuint CreateRenderbuffer(GLsizei samples, gl::TexFormat format, GLsizei width, GLsizei height);
	void DeleteRenderbuffer(GLuint & renderbuffer);

	// -- Buffer API --

	void BindElementArrayBuffer(GLuint buffer);

	void BindDrawIndirectBuffer(GLuint buffer);

	void BindDispatchIndirectBuffer(GLuint buffer);

	void BindUniformBuffer(GLuint index, GLuint buffer);

	void BindTransformFeedbackBuffer(GLuint index, GLuint buffer);

	void BindShaderStorageBuffer(GLuint index, GLuint buffer);

	void BindAtomicCounterBuffer(GLuint index, GLuint buffer);

	// Allocate a new buffer and return a buffer name.
	GLuint CreateBuffer(gl::BufferTarget target, GLsizeiptr size, const GLvoid* data, GLenum usage);

	// Request the deletion of a buffer and set it to zero.
	void DeleteBuffer(GLuint & buffer);

	// Reallocate a buffer. Return the name of a new buffer name might be the same.
	GLuint RecreateBuffer(GLuint buffer, gl::BufferTarget target, GLsizeiptr size, const GLvoid* data, GLenum usage);

	// Update a buffer
	void UploadBufferSubData(GLuint buffer, gl::BufferTarget target, GLintptr offset, GLsizeiptr size, const GLvoid* data);

	// Map a buffer range
	void* MapBuffer(GLuint buffer, gl::BufferTarget target, GLintptr offset, GLsizeiptr length, GLbitfield access);

	// Request the completion of a buffer update
	void UnmapBuffer(GLuint buffer, gl::BufferTarget target);

	// Request a memory flush of CPU memory to GPU memory
	void FlushBuffer(GLuint buffer, gl::BufferTarget target, GLintptr offset, GLsizeiptr length);

	void CopyBufferSubData(GLuint srcBuffer, GLuint dstBuffer, GLintptr srcOffset, GLintptr dstOffset, GLsizeiptr size);

	// Set zeros to an entire buffer
	void ClearBuffer(GLuint buffer, gl::BufferTarget target);

	// Set zeros to buffer data strating from offset for size bytes
	void ClearBufferSubData(GLuint buffer, gl::BufferTarget target, GLintptr offset, GLsizeiptr size);

	// -- Vertex Array API --

	gl::VertexArrayHandle CreateVertexArray();
	void DeleteVertexArray(gl::VertexArrayHandle& vertexArrayName);

	void BindVertexArray(gl::VertexArrayHandle vertexArrayName);
	bool IsVertexArray (gl::VertexArrayHandle vertexArrayName);

	// If vertexArray value is 0 then the default vertex array is bound
	// When the default vertex array is bound the user should user the VertexAttrib*Pointer commands to set the vertex arrays.
	// If a different vertex array is bound, we relly on vertex attrib binding and cache the vertex format.
	void EnableVertexArrayAttrib(GLuint attribIndex, GLuint bufferName, gl::VertexArrayAttribKind Kind, GLint size, VertexChannelFormat format, GLsizei stride, const GLvoid* offset);
	void DisableVertexArrayAttrib(GLuint attribIndex);

	// -- Tessellation API --

	// Set the number of vertices that will be used to make up a single patch primitive when using tessellation
	void SetPatchVertices(int count);

	// -- Stencil API --

	void BindStencilState(const GfxStencilState* state, int stencilRef);

	// -- Enable API --

	// Enable OpenGL capabilities
	void Enable(gl::EnabledCap cap);

	// Disable OpenGL capabilities
	void Disable(gl::EnabledCap cap);

	// Query OpenGL capabilities
	bool IsEnabled(gl::EnabledCap cap) const;

	// -- Misc API --

	void SetPolygonMode(bool wire);
	void SetCullMode(const CullMode cullMode);

	// -- API queries --

	gl::QueryHandle CreateQuery();

	void DeleteQuery(gl::QueryHandle & query);

	void QueryTimeStamp(gl::QueryHandle query);

	GLuint64 Query(gl::QueryHandle query, gl::QueryResult queryResult);

	// Query OpenGL capabilities
	GLint Get(GLenum cap) const;

	// Query whether and OpenGL extension is available
	bool QueryExtension(const char * extension) const;

	// Return the OpenGL extension string
	std::string GetExtensionString() const;

	// Return the OpenGL strings
	const char* GetDriverString(gl::DriverQuery query) const;

	// -- Special functions API --

	// Request to start executing the OpenGL commands. If 'mode' is SUBMIT_FINISH then the function returns when the commands execution has finished.
	void Submit(gl::SubmitMode mode = gl::SUBMIT_FLUSH);

	// Debug only: Check that the actual OpenGL states are equal to the tracked OpenGL states
	bool Verify() const;

	// Check that ApiGLES is in a good state
	bool Check(GLuint name, const char* function, std::size_t line) const;
	template <typename T, typename U, typename V> bool Check(const gl::ObjectHandle<T, U, V>& name, const char* function, std::size_t line) const;

	// Conversions from Unity enums to OpenGL enums
	const TranslateGLES & translate;

private:
	// -- Program Internal --

	GLuint										m_CurrentProgramBinding;
	GLuint										m_CurrentProgramHasTessellation;

	// -- Framebuffer Internal --

	fixed_array<gl::FramebufferHandle, gl::kFramebufferTargetCount>	m_CurrentFramebufferBindings;

	// -- Buffer Internal --

	// Bind a buffer for memory update tasks.
	// This should be entirely handle within ApiGLES
	GLenum BindMemoryBuffer(GLuint buffer, gl::BufferTarget target);

	void UnbindMemoryBuffer(gl::BufferTarget target);

	void BindReadBuffer(GLuint buffer);

	// Bind a buffer for vertex attrib update or memory update tasks.
	// This should be entirely handle within ApiGLES
	void BindArrayBuffer(GLuint buffer);

	fixed_array<GLuint, gl::kBufferTargetSingleBindingCount>		m_CurrentBufferBindings; // Store the state caching for all the buffer bindings that have only a single binding per target.
	fixed_array<GLuint, gl::kMaxUniformBufferBindings>				m_CurrentUniformBufferBindings;
	fixed_array<GLuint, gl::kMaxTransformFeedbackBufferBindings>	m_CurrentTransformBufferBindings;
	fixed_array<GLuint, gl::kMaxShaderStorageBufferBindings>		m_CurrentStorageBufferBindings;
	fixed_array<GLuint, gl::kMaxAtomicCounterBufferBindings>		m_CurrentAtomicCounterBufferBindings;

	// -- Vertex Array Internal --

	// OpenGL core profile doesn't support the default vertex array 0. Hence, we create a default one
	// and store it into m_DefaultVertexArrayName
	// If the legacy vertex attributes are called then we use the default vertex array.
	// http://www.opengl.org/registry/specs/ARB/vertex_array_object.txt
	
	// If the vertex attrib binding function are called, then we used a named vertex array
	// http://www.opengl.org/registry/specs/ARB/vertex_attrib_binding.txt

	// The value for the currently bound vertex array object
	gl::VertexArrayHandle										m_CurrentVertexArrayBinding;

	class VertexArray
	{
	public:
		VertexArray();
		VertexArray(const ApiGLES & api, GLuint buffer, GLint size, VertexChannelFormat format, GLsizei stride, const GLvoid* offset, gl::VertexArrayAttribKind kind);

	private:
		const GLvoid* m_Offset;
		GLsizei m_Stride;
		GLuint m_Buffer;
		unsigned char m_Bitfield;
	};

	fixed_array<VertexArray, gl::kVertexAttrCount>	m_CurrentDefaultVertexArray;
	gl::VertexArrayHandle							m_DefaultVertexArrayName;
	unsigned int									m_CurrentDefaultVertexArrayEnabled;

	// -- Rasterizer Internal --

	::CullMode										m_CurrentCullMode;

	// -- Tessellation Internal --

	GLint											m_CurrentPatchVertices;

	// -- Stencil Internal --

	const GfxStencilState*							m_CurrentStencilState;
	int												m_CurrentStencilRef;

	// -- Enable / Disable --

	UInt64											m_CurrentCapEnabled;

	// -- Misc --
	bool											m_CurrentPolygonModeWire;

private:
	GLuint													m_CurrentTextureUnit;
	// -- Sampler --

	fixed_array<GLuint, gl::kMaxTextureBindings>	m_CurrentSamplerBindings;

	// -- All initialization time code --

#	if SUPPORT_THREADS
		Thread::ThreadID							m_Thread;
#	endif

	void Load(GfxDeviceLevelGL contextLevel);

	bool											m_Caching;
};

template <typename T, typename U, typename V>
bool ApiGLES::Check(const gl::ObjectHandle<T, U, V>& name, const char* function, std::size_t line) const
{
	return Check(GLES_OBJECT_NAME(name), function, line);
}

extern ApiGLES* gGL;
