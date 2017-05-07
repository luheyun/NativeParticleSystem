#pragma once

#include "HandleGLES.h"
#include "FrameBufferGLES.h"
#include <map>

class ApiGLES;
struct RenderSurfaceGLES;

class GfxContextGLES : NonCopyable
{
public:
	class Instance : NonCopyable
	{
		friend class GfxContextGLES;

	public:
		Instance(void *context, ApiGLES & Api);
		~Instance();

	public:
		GfxFramebufferGLES& GetFramebuffer();
		const GfxFramebufferGLES& GetFramebuffer() const;

		void Invalidate();

	private:
		ApiGLES &m_Api;
		GfxFramebufferGLES m_Framebuffer;
		gl::VertexArrayHandle m_DefaultVertexArrayName; // OpenGL ES 3.0 indirect draw and OpenGL core profile requires a default vertex array object that is not 0
	};

public:
	GfxContextGLES();
	~GfxContextGLES();

	// Access to the current context instance
	Instance& GetCurrent() const;

	// Make current the context instance associated with "context"
	Instance& MakeCurrent(ApiGLES & api, void* context);
	
	// Invalidate all the states of all the contexts alive
	void Invalidate(ApiGLES & api);

	// Register the texture ID/renderbuffer ID(s) from this rendersurface to the deferred FBO invalidate list of each context.
	void AddRenderSurfaceToDeferredFBOInvalidateList(const RenderSurfaceGLES *rs);

	// Access to the framebuffer manager and the default FBO of the current context
	GfxFramebufferGLES& GetFramebuffer();
	const GfxFramebufferGLES& GetFramebuffer() const;
	gl::VertexArrayHandle GetDefaultVertexArray() const;
	gl::FramebufferHandle GetDefaultFBO() const;

private:
	typedef std::pair<void*, Instance*> Pair;
	typedef std::map<void*, Instance*> Map;
};
