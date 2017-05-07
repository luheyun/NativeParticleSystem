#include "UnityPrefix.h"

#include "GfxContextGLES.h"
#include "ApiGLES.h"
#include "AssertGLES.h"
#include "Runtime/GfxDevice/TextureIdMap.h"
#include "Runtime/Shaders/GraphicsCaps.h"

#if FORCE_DEBUG_BUILD_WEBGL
#	undef UNITY_WEBGL
#	define UNITY_WEBGL 1
#endif//FORCE_DEBUG_BUILD_WEBGL

GfxContextGLES::Instance::Instance(void *context, ApiGLES & Api)
	: m_Context(context)
	, m_Api(Api)
	, m_Framebuffer(Api, this)
	, m_DefaultVertexArrayName(GetGraphicsCaps().gles.hasVertexArrayObject ? gGL->CreateVertexArray() : gl::VertexArrayHandle())
{}

GfxContextGLES::Instance::~Instance()
{
	if (GetGraphicsCaps().gles.hasVertexArrayObject && m_DefaultVertexArrayName != 0)
		gGL->DeleteVertexArray(m_DefaultVertexArrayName);
}

GfxFramebufferGLES& GfxContextGLES::Instance::GetFramebuffer()
{
	return m_Framebuffer;
}

const GfxFramebufferGLES& GfxContextGLES::Instance::GetFramebuffer() const
{
	return m_Framebuffer;
}

void GfxContextGLES::Instance::Invalidate()
{
	m_Framebuffer.Invalidate();
	gGL->BindVertexArray(m_DefaultVertexArrayName);
}

GfxContextGLES::GfxContextGLES()
{}

GfxContextGLES::~GfxContextGLES()
{
	// The GfxDevice is deleted before the surfaces when exiting Unity
#	if 0
		for (Map::iterator it = m_ContextMap.begin(); it != m_ContextMap.end(); ++it)
			delete it->second;
#	endif
}

GfxContextGLES::Instance& GfxContextGLES::GetCurrent() const
{
	// GfxContextGLES::MakeCurrent needs to be called before using a current context
	Assert(m_CurrentContext.second);

	return *m_CurrentContext.second;
}

GfxContextGLES::Instance& GfxContextGLES::MakeCurrent(ApiGLES & api, void* context)
{
	Assert(context);
	AssertFormatMsg(context == (void*)1 || reinterpret_cast<void*>(gl::GetCurrentContext()) == context, "The new context (%p) must already be the current OpenGL context (%p)", context, (void*)gl::GetCurrentContext());
	
	if (context == (void*)1 && !m_ContextMap.empty())
	{
		// If context hold 1, we are asking for the master context
		m_CurrentContext = m_MasterContext;
	}
	else if (m_CurrentContext.first == context)
	{
		// Check if the requested context is already current
	}
	else
	{
		// Search for an existing context
		Map::iterator it = m_ContextMap.find(context);

		if (it != m_ContextMap.end())
		{
			// The GfxContextGLES context already exist for the "context" OpenGL context
			m_CurrentContext = std::make_pair(context, it->second);
		}
		else
		{
			bool IsCreatingMaster = m_ContextMap.empty();

			// We create a new GfxContextGLES context associated with the "context" OpenGL context
			std::pair<Map::iterator, bool> itInsert = m_ContextMap.insert(std::make_pair(context, new Instance(context, api)));
			Assert(itInsert.first != m_ContextMap.end());
			m_CurrentContext = std::make_pair(context, itInsert.first->second);
			if (IsCreatingMaster)
				m_MasterContext = m_CurrentContext;
		}
	}

	DebugAssertMsg(m_CurrentContext.first && m_CurrentContext.second, "OPENGL ERROR: Current Context not initialized");
	DebugAssertMsg(m_MasterContext.first && m_MasterContext.second, "OPENGL ERROR: Master Context not initialized");

	api.Invalidate(*this);

	return *m_CurrentContext.second;
}

void GfxContextGLES::Invalidate(ApiGLES & api)
{
	// Invalidate each individual context
	for (Map::iterator it = m_ContextMap.begin(); it != m_ContextMap.end(); ++it)
	{
		DebugAssertMsg(it->second, "OPENGL ERROR: The OpenGL context doesn't exist or has been destroyed");
		it->second->Invalidate();
	}

	// There is no current context
	m_CurrentContext = std::make_pair(reinterpret_cast<void*>(NULL), reinterpret_cast<Instance*>(NULL));
}

GfxFramebufferGLES& GfxContextGLES::GetFramebuffer()
{
	return this->GetCurrent().GetFramebuffer();
}

const GfxFramebufferGLES& GfxContextGLES::GetFramebuffer() const
{
	return this->GetCurrent().GetFramebuffer();
}

gl::VertexArrayHandle GfxContextGLES::GetDefaultVertexArray() const
{
	gl::VertexArrayHandle defaultVertexArrayName = this->GetCurrent().m_DefaultVertexArrayName;
	if (GetGraphicsCaps().gles.hasVertexArrayObject && gGL && !gGL->IsVertexArray (defaultVertexArrayName))
		// FIXME: This should never be true, but currently it often is, when switching contexts
		// Force-recreate default vertex array name on the currently bound context
		defaultVertexArrayName = this->GetCurrent().m_DefaultVertexArrayName = gGL->CreateVertexArray ();
	return defaultVertexArrayName;
}

gl::FramebufferHandle GfxContextGLES::GetDefaultFBO() const
{
	return this->GetCurrent().GetFramebuffer().GetDefaultFBO();
}

void GfxContextGLES::AddRenderSurfaceToDeferredFBOInvalidateList(const RenderSurfaceGLES *rs)
{
	for (Map::iterator it = m_ContextMap.begin(); it != m_ContextMap.end(); ++it)
	{
		// Skip the currently active one, that's going to get cleaned up directly
		if (m_CurrentContext.first == it->first)
			continue;

		it->second->m_Framebuffer.AddRenderSurfaceToDeferredFBOInvalidateList(rs);
	}
}