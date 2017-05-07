#include "UnityPrefix.h"

#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "FrameBufferGLES.h"
#include "AssertGLES.h"
#include "TextureIdMapGLES.h"
#include "TexturesGLES.h"
#include "GraphicsCapsGLES.h"
#include "GfxDeviceGLES.h"
#include "GfxContextGLES.h"
#include "GfxDeviceGLES.h"

#include "Runtime/Shaders/GraphicsCaps.h"
#include "Runtime/GfxDevice/GfxDevice.h"
#include "Runtime/Graphics/ScreenManager.h"

#define DEBUG_GLES_FRAMEBUFFER 0

#if DEBUG_GLES_FRAMEBUFFER
#include <sstream>
#endif
#if FORCE_DEBUG_BUILD_WEBGL
#	undef UNITY_WEBGL
#	define UNITY_WEBGL 1
#endif//FORCE_DEBUG_BUILD_WEBGL

namespace
{
#if DEBUG_GLES_FRAMEBUFFER
	std::string DescribeRT(const GLESRenderTargetSetup &rt)
	{
		std::ostringstream oss;
		oss << "[ ";
		for (int i = 0; i < rt.m_ColorCount; i++)
		{
			oss << "col" << i << ": ";
			if (rt.m_ColorTexIDs[i].m_ID != 0)
				oss << "tex " << rt.m_ColorTexIDs[i].m_ID;
			else
				oss << "rb " << rt.m_ColorRBIDs[i];
			oss << "\t";
		}
		
		if (rt.m_HasDepth)
		{
			oss << "depth: ";
			if (rt.m_DepthTexID.m_ID != 0)
				oss << "tex " << rt.m_DepthTexID.m_ID;
			else
				oss << "rb " << rt.m_DepthRBID;
		}

		oss << " ]";
		return std::string(oss.str().c_str());
	}
#endif

	inline void GetRenderSurfaceName(ApiGLES * api, RenderSurfaceGLES* rs, char* name, size_t len)
	{
		if (rs->textureID.m_ID)
			api->GetDebugLabel(gl::kTexture, ((GLESTexture*)TextureIdMap::QueryNativeTexture(rs->textureID))->texture, len, 0, name);
		else if (rs->buffer)
			api->GetDebugLabel(gl::kRenderbuffer, rs->buffer, len, 0, name);
		else
			name[0] = '\0';
	}

	void TryNamingFBOFromAttachments(ApiGLES * api, gl::FramebufferHandle fb, const GfxRenderTargetSetup& attach)
	{
		// we will compare strings, yes: it is one time operation so hashing wont help
		char nameFirst[128];
		char nameCheck[128];
		bool nameInited = false;

		for(int i = 0, n = attach.colorCount ; i < n ; ++i)
		{
			RenderSurfaceBase* rs = attach.color[i];
			Assert(rs);

			if(!gles::IsDummySurface(rs))
			{
				GetRenderSurfaceName(api, (RenderSurfaceGLES *)rs, nameCheck, sizeof(nameCheck));
				if(!nameInited)
				{
					::memcpy(nameFirst, nameCheck, sizeof(nameFirst));
					nameInited = true;
					continue;
				}
				if(::strcmp(nameFirst, nameCheck) != 0)
					return;
			}
		}

		if (attach.depth)
		{
			// special care for depth-only RT (nameInited will be false)
			GetRenderSurfaceName(api, (RenderSurfaceGLES *)attach.depth, nameCheck, sizeof(nameCheck));
			if (nameInited && ::strcmp(nameFirst, nameCheck) != 0)
				return;
			nameInited = true;
		}

		if (nameInited)
			api->DebugLabel(gl::kFramebuffer, GLES_OBJECT_NAME(fb), 0, nameCheck);
	}

	void SetupDrawBuffers(ApiGLES& api, gl::FramebufferHandle framebufferName, const GfxRenderTargetSetup& attach)
	{
		if (attach.colorCount == 0)
		{
			if (g_GraphicsCapsGLES->requireDrawBufferNone)
				api.BindFramebufferDrawBuffers(framebufferName, 1, &GL_NONE);
		}
		else if (g_GraphicsCapsGLES->hasDrawBuffers)
		{
			fixed_array<GLenum, kMaxSupportedRenderTargets> drawBuffers;
			for (int i = 0; i < attach.colorCount; ++i)
				drawBuffers[i] = gles::IsDummySurface(attach.color[i]) ? GL_NONE : GL_COLOR_ATTACHMENT0+i;

			api.BindFramebufferDrawBuffers(framebufferName, attach.colorCount, &drawBuffers[0]);
		}
	}

	gl::FramebufferHandle CreateFramebuffer(ApiGLES & api, const GfxRenderTargetSetup& attach)
	{
		gl::FramebufferHandle framebufferName = api.CreateFramebuffer();

		const GLenum target = GetGraphicsCaps().gles.framebufferTargetForBindingAttachments;
		gl::FramebufferHandle restoreFramebufferName = api.GetFramebufferBinding(gl::kDrawFramebuffer);

		DebugAssert(attach.colorCount <= GetGraphicsCaps().maxMRTs);
		DebugAssert(!attach.color[0] || !attach.color[0]->backBuffer);
		DebugAssert(!attach.depth || !attach.depth->backBuffer);

		api.BindFramebuffer(gl::kDrawFramebuffer, framebufferName);
		for(int i = 0, n = attach.colorCount ; i < n ; ++i)
		{
			GLenum				colorAttach	= GL_COLOR_ATTACHMENT0 + i;
			RenderSurfaceGLES*	color = (RenderSurfaceGLES *)attach.color[i];
			DebugAssert(color);

			if (color->flags & kSurfaceCreateNeverUsed)
				continue;

			GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(color->textureID);

			bool hasTexture = false;
			if (texInfo)
				hasTexture = (texInfo->texture && color->format != gl::kTexFormatNone);

			if (hasTexture)
			{
				int mipLevel = attach.mipLevel;
				if (mipLevel != 0 && !GetGraphicsCaps().gles.supportsManualMipmaps)
				{
					WarningString("Warning: Rendering to mipmap levels other than zero is not supported on this device");
					mipLevel = 0;
				}

				if (color->dim == kTexDimCUBE)
					GLES_CALL(&api, glFramebufferTexture2D, target, colorAttach, GL_TEXTURE_CUBE_MAP_POSITIVE_X + clamp<int>(attach.cubemapFace, 0, 5), texInfo->texture, mipLevel);
				else if (color->dim == kTexDim3D || color->dim == kTexDim2DArray)
				{
					int depthSlice = attach.depthSlice;
					// before GL3.2 / GLES3.2 ability to bind whole 3D/2DArray texture does not exist,
					// so just bind slice 0 in that case
					if (depthSlice == -1 && !api.glFramebufferTexture)
					{
						depthSlice = 0;
					}
					if (depthSlice == -1)
						GLES_CALL(&api, glFramebufferTexture, target, colorAttach, texInfo->texture, mipLevel);
					else
						GLES_CALL(&api, glFramebufferTextureLayer, target, colorAttach, texInfo->texture, mipLevel, depthSlice);
				}
				else if(color->samples > 1 && GetGraphicsCaps().hasMultiSampleAutoResolve)
					GLES_CALL(&api, glFramebufferTexture2DMultisampleEXT, target, colorAttach, GL_TEXTURE_2D, texInfo->texture, mipLevel, color->samples);
				else
					GLES_CALL(&api, glFramebufferTexture2D, target, colorAttach, GL_TEXTURE_2D, texInfo->texture, mipLevel);
			}
			else
			{
				GLES_CALL(&api, glFramebufferRenderbuffer, target, colorAttach, GL_RENDERBUFFER, color->buffer);
			}
		}

		RenderSurfaceGLES* depth = (RenderSurfaceGLES*)attach.depth;
		if (depth && !(depth->flags & kSurfaceCreateNeverUsed))
		{
			GLESTexture* texInfo = (GLESTexture*)TextureIdMap::QueryNativeTexture(attach.depth->textureID);
			bool needSetupDepthTex = false;
			if (texInfo)
				needSetupDepthTex = (texInfo->texture != 0);

			bool needAttachStencil = GetGraphicsCaps().hasStencil && gl::IsFormatDepthStencil(depth->format);

#		if UNITY_WEBGL
			// WebGL differs from GLES in that it requires handling stencil and depth as a single attachment.
			// See: https://www.khronos.org/registry/webgl/specs/latest/1.0/#6.6
			if (needAttachStencil)
			{
				if(needSetupDepthTex)
					GLES_CALL(&api, glFramebufferTexture2D, target, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texInfo->texture, 0);
				else
					GLES_CALL(&api, glFramebufferRenderbuffer, target, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth->buffer);
			}
			else
#		endif
			if (needSetupDepthTex)
			{
				GLES_CALL(&api, glFramebufferTexture2D, target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texInfo->texture, 0);
				if (needAttachStencil && GetGraphicsCaps().hasRenderTargetStencil)
				{
					if (depth->stencilBuffer == 0)
					GLES_CALL(&api, glFramebufferTexture2D, target, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texInfo->texture, 0);
				else
						GLES_CALL(&api, glFramebufferRenderbuffer, target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth->stencilBuffer);
				}
				else
					GLES_CALL(&api, glFramebufferTexture2D, target, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
				
			}
			else
			{
				GLES_CALL(&api, glFramebufferRenderbuffer, target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth->buffer);
				if (needAttachStencil)
					GLES_CALL(&api, glFramebufferRenderbuffer, target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth->stencilBuffer ? depth->stencilBuffer : /*packed depth+stencil*/depth->buffer);
				else
					GLES_CALL(&api, glFramebufferRenderbuffer, target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}
		}

		SetupDrawBuffers(api, framebufferName, attach);

		DebugAssertMsg(api.CheckFramebuffer(framebufferName), "OPENGL ERROR: The framebuffer is not complete");

		if (g_GraphicsCapsGLES->hasDebugLabel)
			TryNamingFBOFromAttachments(&api, framebufferName, attach);

		api.BindFramebuffer(gl::kDrawFramebuffer, restoreFramebufferName);

#if DEBUG_GLES_FRAMEBUFFER
		printf_console("*** GLES: FBO create: %d: %s\n", GLES_OBJECT_NAME(framebufferName), DescribeRT(GLESRenderTargetSetup(attach)).c_str());
#endif

		return framebufferName;
	}

	bool HasAsAttachment(const GLESRenderTargetSetup& attach, RenderSurfaceBase* rs)
	{
		RenderSurfaceGLES *r = (RenderSurfaceGLES *)rs;

		if (attach.m_HasDepth)
		{
			if ( (attach.m_DepthTexID.IsValid() && attach.m_DepthTexID == r->textureID) ||
				 (attach.m_DepthRBID && attach.m_DepthRBID == r->buffer) ||
				 (attach.m_DepthStencilID && attach.m_DepthStencilID == r->stencilBuffer))
				return true;
		}

		for (unsigned i = 0; i < attach.m_ColorCount; i++)
		{
			if((attach.m_ColorTexIDs[i].IsValid() && (attach.m_ColorTexIDs[i] == r->textureID)) ||
				(attach.m_ColorRBIDs[i] && attach.m_ColorRBIDs[i] == r->buffer))
				return true;
		}

		return false;
	}

	bool HasAsAttachment(const GLESRenderTargetSetup& attach, const TextureID &tid)
	{
		DebugAssert(tid.IsValid());

		if (attach.m_HasDepth)
		{
			if (attach.m_DepthTexID == tid)
				return true;
		}

		for (unsigned i = 0; i < attach.m_ColorCount; i++)
		{
			if (attach.m_ColorTexIDs[i] == tid)
				return true;
		}

		return false;
	}

	bool HasAsAttachment(const GLESRenderTargetSetup& attach, const GLuint rbid)
	{
		DebugAssert(rbid != 0);

		if (attach.m_HasDepth)
		{
			if ((attach.m_DepthRBID == rbid) ||
				(attach.m_DepthStencilID == rbid))
				return true;
		}

		for (unsigned i = 0; i < attach.m_ColorCount; i++)
		{
			if (attach.m_ColorRBIDs[i] == rbid)
				return true;
		}

		return false;
	}

	// Remove all occurences of rs from renderTarget.
	// This may change the number of colorCount of renderTarget and depth may be NULL.
	// Returns true if renderTarget was modified.
	bool InvalidateSurfacePtr(GfxRenderTarget& renderTarget, const RenderSurfaceBase* rs)
	{
		const int oldColorCount = renderTarget.colorCount;
		renderTarget.colorCount = std::remove(renderTarget.color, renderTarget.color + oldColorCount, rs) - renderTarget.color;
		bool hadInvalidPtr = (oldColorCount != renderTarget.colorCount);
		
		if (renderTarget.depth == rs)
		{
			renderTarget.depth = NULL;
			hadInvalidPtr = true;
		}
		return hadInvalidPtr;
	}

}//namespace

namespace gles
{
	namespace internal
	{
		void FillRenderTargetSetup(GfxRenderTargetSetup *setup, RenderSurfaceBase *col, RenderSurfaceBase *depth)
		{
			::memset(setup, 0x00, sizeof(GfxRenderTargetSetup));
			setup->color[0] = col;
			setup->depth = depth;
			setup->colorCount = col ? 1 : 0;
			setup->colorLoadAction[0] = kGfxRTLoadActionLoad;
			setup->colorStoreAction[0] = kGfxRTStoreActionStore;
			setup->depthLoadAction = kGfxRTLoadActionLoad;
			setup->depthStoreAction = kGfxRTStoreActionStore;
			setup->cubemapFace = kCubeFaceUnknown;
			setup->mipLevel = 0;
			setup->flags = 0;
		}
	}
	void FillRenderTargetSetup(GfxRenderTargetSetup *setup, RenderSurfaceBase *col, RenderSurfaceBase *depth)
	{
		Assert(setup && col && depth);
		internal::FillRenderTargetSetup(setup, col, depth);
	}

} // namespace


GfxFramebufferGLES::GfxFramebufferGLES(ApiGLES & api, void* context)
	: m_FramebufferMap()
	, m_FramebufferSamplesCount(0)
	, m_CurrentFramebufferSetup()
	, m_CurrentFramebufferValid(false)
	, m_PendingFramebufferSetup()
	, m_PendingFramebufferValid(false)
	, m_RequiresFramebufferSetup(true)
	, m_Context(context)
	, m_Api(api)
	, m_BlitQuad()
	, m_DefaultFBO()
#if USES_GLES_DEFAULT_FBO
	, m_DefaultGLESFBOInited(false)
#endif
{
	memset(&m_BackBufferColorSurface, 0, sizeof(RenderSurfaceGLES));
	memset(&m_BackBufferDepthSurface, 0, sizeof(RenderSurfaceGLES));
	using gles::internal::FillRenderTargetSetup;
	FillRenderTargetSetup(&m_DefaultFramebuffer, &m_BackBufferColorSurface, &m_BackBufferDepthSurface);
	FillRenderTargetSetup(&m_CurrentFramebuffer, &m_BackBufferColorSurface, &m_BackBufferDepthSurface);
	FillRenderTargetSetup(&m_PendingFramebuffer, &m_BackBufferColorSurface, &m_BackBufferDepthSurface);
}

void GfxFramebufferGLES::SetupDefaultFramebuffer(RenderSurfaceBase** outColor, RenderSurfaceBase** outDepth, gl::FramebufferHandle inFbo)
{
	RenderSurfaceBase_InitColor(m_BackBufferColorSurface);
	m_BackBufferColorSurface.backBuffer = true;

	RenderSurfaceBase_InitDepth(m_BackBufferDepthSurface);
	m_BackBufferDepthSurface.backBuffer = true;

	// fill out and register default fbo
	gles::FillRenderTargetSetup(&m_DefaultFramebuffer, &m_BackBufferColorSurface, &m_BackBufferDepthSurface);

	GLESRenderTargetSetup setup(m_DefaultFramebuffer);
	// In case of non-0 FBO this may well already exist in the map, but do it just in case.
	m_FramebufferMap[setup] = inFbo;

	m_DefaultFBO = inFbo;
	this->UpdateDefaultFramebufferViewport();

	if(outColor)	*outColor = &m_BackBufferColorSurface;
	if(outDepth)	*outDepth = &m_BackBufferDepthSurface;
}

void GfxFramebufferGLES::UpdateDefaultFramebuffer(RenderSurfaceBase* color, RenderSurfaceBase* depth, gl::FramebufferHandle inFbo)
{
	// in gfx device we keep *pointers* to BB render surfaces, and we can query and keep them in outside code
	// so update (copy) our back-buffer RS instead
	m_BackBufferColorSurface	= *(RenderSurfaceGLES*)color;
	m_BackBufferDepthSurface	= *(RenderSurfaceGLES*)depth;

	gles::FillRenderTargetSetup(&m_DefaultFramebuffer, &m_BackBufferColorSurface, &m_BackBufferDepthSurface);
	GLESRenderTargetSetup setup(m_DefaultFramebuffer);
	m_FramebufferMap[setup] = inFbo;
	m_DefaultFBO = inFbo;
}

void GfxFramebufferGLES::SetBackBufferColorDepthSurface(RenderSurfaceBase* color, RenderSurfaceBase *depth)
{
	m_BackBufferColorSurface = *(RenderSurfaceGLES*)color;
	m_BackBufferDepthSurface = *(RenderSurfaceGLES*)depth;
	gles::FillRenderTargetSetup(&m_DefaultFramebuffer, &m_BackBufferColorSurface, &m_BackBufferDepthSurface);
	GLESRenderTargetSetup setup(m_DefaultFramebuffer);
	gl::FramebufferHandle fbo = GetFramebufferName(m_DefaultFramebuffer);
	m_FramebufferMap[setup] = fbo;
	m_DefaultFBO = fbo;
}

void GfxFramebufferGLES::UpdateDefaultFramebufferViewport()
{
#if UNITY_ANDROID || UNITY_BB10 || UNITY_WIN || UNITY_TIZEN || UNITY_OSX || UNITY_LINUX
	if (GetScreenManagerPtr())
	{
		// do this in a more proper way.. laterTM
		const Rectf window = GetScreenManager().GetRect();

		// TODO: take care about external FBO
		// for now this is android-only code, so no-external default fbo
		m_BackBufferColorSurface.width	= m_BackBufferDepthSurface.width	= window.width;
		m_BackBufferColorSurface.height	= m_BackBufferDepthSurface.height	= window.height;
	}
#endif//UNITY_ANDROID || UNITY_BB10 || UNITY_WIN || UNITY_TIZEN || UNITY_OSX || UNITY_LINUX
}

void GfxFramebufferGLES::Activate(const Builtin builtin, const bool clear)
{
	GfxRenderTargetSetup* active = NULL;

	switch (builtin)
	{
	case kDefault:
		active = &m_DefaultFramebuffer;
		break;
	case kPending:
		active = &m_PendingFramebuffer;
		break;
	case kCurrent:
		active = &m_CurrentFramebuffer;
		break;
	default:
		DebugAssert(0);
	}

	DebugAssert(active);

	if (clear)
	{
		active->color[0]->loadAction = active->colorLoadAction[0] = kGfxRTLoadActionDontCare;
		active->depth->loadAction = active->depthLoadAction = kGfxRTLoadActionDontCare;
	}
	this->Activate(*active);
}

void GfxFramebufferGLES::Activate(const GfxRenderTargetSetup& attach)
{
	DebugAssert(attach.color[0] && attach.depth);
	DebugAssert(attach.color[0]->backBuffer == attach.depth->backBuffer);

#if USES_GLES_DEFAULT_FBO
	this->EnsureDefaultFBOInited();
#endif
	this->InvalidateAttachments();

	m_PendingFramebuffer = attach;
	m_PendingFramebufferValid = true;

#if UNITY_IOS || UNITY_TVOS
	// i dont know why the code below is needed at all (and gl couldnt quite help)
	// but i do know that on ios/tvos we have quite special backbuffer handling:
	// we actually have 2 backbuffers: one "unity" backbuffer and another one "system"
	// this is needed because ios cannot change native resolution, so we do resize "unity" backbuffer RT and blit at frame end
	// now we DO want unity to think that "unity" backbuffer is actuall a backbuffer, hence we set all these flags
	// but when blitting to "system" framebuffer the code below will change it to "unity" backbuffer hence breaking everything
	// and we dont want to tweak what GfxFramebufferGLES think is backbuffer back-and-forth
	// hence this fancy magic is disabled on ios/tvos
#else
	// When using MT renderer, the GfxDevice::m_BackBufferColorSurface owned by GfxDeviceClient won't have the width and height data set up properly.
	// As a workaround, use the proper backbuffer surfaces.
	if (attach.color[0] && attach.color[0]->backBuffer)
	{
		m_PendingFramebuffer.color[0] = &m_BackBufferColorSurface;
	}
	// Same for depth
	if (attach.depth && attach.depth->backBuffer)
	{
		m_PendingFramebuffer.depth = &m_BackBufferDepthSurface;
	}
#endif

	m_RequiresFramebufferSetup = true;

	m_PendingFramebufferSetup.scissor.x = m_PendingFramebufferSetup.viewport.x = 0;
	m_PendingFramebufferSetup.scissor.y = m_PendingFramebufferSetup.viewport.y = 0;
	m_PendingFramebufferSetup.scissor.width = m_PendingFramebufferSetup.viewport.width = m_PendingFramebuffer.color[0]->width;
	m_PendingFramebufferSetup.scissor.height = m_PendingFramebufferSetup.viewport.height = m_PendingFramebuffer.color[0]->height;
	m_PendingFramebufferSetup.fbo = this->GetFramebufferName(m_PendingFramebuffer);
}

void GfxFramebufferGLES::MakeCurrentFramebuffer(const Builtin builtin)
{
	switch(builtin)
	{
		case kDefault:
			m_CurrentFramebuffer = m_DefaultFramebuffer;
			m_CurrentFramebufferSetup.fbo = m_DefaultFBO;
			break;
		case kPending:
			m_CurrentFramebuffer = m_PendingFramebuffer;
			m_CurrentFramebufferSetup.fbo = m_PendingFramebufferSetup.fbo;
			break;
		case kCurrent:
			break;
	}

	m_CurrentFramebufferValid = true;
	m_Api.BindFramebuffer(gl::kDrawFramebuffer, m_CurrentFramebufferSetup.fbo);

#if UNITY_WEBGL // workaround, it seems that glDrawBuffers state is not part of the framebuffer state in the WebGL impl. used by graphics tests (Mozilla).
	// SetupDrawBuffers handles only FBOs, not the backbuffer
	if (m_CurrentFramebufferSetup.fbo != 0)
	{
		SetupDrawBuffers(m_Api, m_CurrentFramebufferSetup.fbo, m_CurrentFramebuffer);
	}
#endif

#if DEBUG_GLES_FRAMEBUFFER
	printf_console("GLES: FBO activate: %d: %s\n", GLES_OBJECT_NAME(m_CurrentFramebufferSetup.fbo), DescribeRT(GLESRenderTargetSetup(m_CurrentFramebuffer)).c_str());
#endif

	this->ApplyViewport();
	this->ApplyScissor();

	DebugAssertMsg(m_Api.GetFramebufferBinding(gl::kDrawFramebuffer) == m_CurrentFramebufferSetup.fbo, "GfxFramebufferGLES: Inconsistent framebuffer setup");
}

void GfxFramebufferGLES::InvalidateAttachments(const bool invalidateColor[kMaxSupportedRenderTargets], bool invalidateDepth)
{
	DebugAssertMsg(m_Api.GetFramebufferBinding(gl::kDrawFramebuffer) == m_CurrentFramebufferSetup.fbo, "GfxFramebufferGLES: Inconsistent framebuffer setup");

	if (GetGraphicsCaps().gles.hasInvalidateFramebuffer)
	{
		GLenum	invalidateTarget[kMaxSupportedRenderTargets+2] = {0};
		int		invalidateCount = 0;

		const bool systemFbo = m_CurrentFramebufferSetup.fbo == 0;

		for (int i = 0, n = m_CurrentFramebuffer.colorCount ; i < n ; ++i)
		{
			if(invalidateColor[i])
				invalidateTarget[invalidateCount++] = systemFbo ? GL_COLOR : GL_COLOR_ATTACHMENT0 + i;
		}

		if (invalidateDepth && systemFbo)
		{
			// glInvalidateFramebuffer should ignore specified attachments that are not present in the actual FBO
			// but this is not the case for the back buffer with at least some drivers.
			invalidateDepth =	m_CurrentFramebuffer.depth &&
								(m_Api.translate.GetFormatDesc(static_cast<RenderSurfaceGLES*>(m_CurrentFramebuffer.depth)->format).flags & gl::kTextureCapDepth);
		}

		if (invalidateDepth)
		{
			invalidateTarget[invalidateCount++] = systemFbo ? GL_DEPTH : GL_DEPTH_ATTACHMENT;
			invalidateTarget[invalidateCount++] = systemFbo ? GL_STENCIL : GL_STENCIL_ATTACHMENT;
		}

		if (invalidateCount > 0)
			GLES_CALL(&m_Api, glInvalidateFramebuffer, GL_FRAMEBUFFER, invalidateCount, invalidateTarget);
	}
}

void GfxFramebufferGLES::InvalidateAttachments()
{
	bool discardColor[kMaxSupportedRenderTargets] = {0};
	int discardCount = 0;
	for (int i = 0, n = m_CurrentFramebuffer.colorCount ; i < n ; ++i)
	{
		if (!m_CurrentFramebuffer.color[i])
			continue;

		discardColor[i] = (m_CurrentFramebuffer.color[i]->storeAction == kGfxRTStoreActionDontCare);
		m_CurrentFramebuffer.color[i]->storeAction = kGfxRTStoreActionStore;
		if(discardColor[i])
			discardCount++;
	}

	bool discardDepth = false;
	if (m_CurrentFramebuffer.depth)
	{
		discardDepth = (m_CurrentFramebuffer.depth->storeAction == kGfxRTStoreActionDontCare);
		m_CurrentFramebuffer.depth->storeAction = kGfxRTStoreActionStore;
		if(discardDepth)
			discardCount++;
	}
	if(discardCount > 0)
		this->InvalidateAttachments(discardColor, discardDepth);
}

void GfxFramebufferGLES::TryInvalidateDefaultFramebufferDepth()
{
	if (!g_GraphicsCapsGLES->hasInvalidateFramebuffer)
		return;

	if (m_CurrentFramebufferSetup.fbo != m_DefaultFBO)
		return;

	const bool dontDiscardColor[kMaxSupportedRenderTargets] = {};
	const bool discardDepth = true;
	this->InvalidateAttachments(dontDiscardColor, discardDepth);
}

void GfxFramebufferGLES::ReleaseFramebuffer(RenderSurfaceBase* rs, GfxContextGLES *contexts)
{
	GLES_ASSERT(&m_Api, !HasAsAttachment(GLESRenderTargetSetup(m_DefaultFramebuffer), rs), "Cannot delete surface of default framebuffer");

	// When deleting a rendersurface, we'll need to notify all other GfxFramebufferGLES
	// instances in other contexts as well that a surface is no more (for both textures and rendersurfaces).
	if(contexts)
		contexts->AddRenderSurfaceToDeferredFBOInvalidateList((RenderSurfaceGLES *)rs);

	// Also cancel any possible mip gens.
	static_cast<GfxDeviceGLES &>(GetRealGfxDevice()).CancelPendingMipGen(rs);

	// update cache
	gl::FramebufferHandle curFB = m_Api.GetFramebufferBinding(gl::kDrawFramebuffer);

	// When we release the current framebuffer, we don't rebind the current framebuffer and let the active FBO code figure it out
	bool rebindCurrentFramebuffer = true;

	for (FramebufferMap::iterator fbi = m_FramebufferMap.begin(), fbend = m_FramebufferMap.end() ; fbi != fbend ; )
	{
		if (::HasAsAttachment(fbi->first, rs))
		{

#if DEBUG_GLES_FRAMEBUFFER
			printf_console("*** GLES FBO Delete %d %s\n", GLES_OBJECT_NAME(fbi->second), DescribeRT(GLESRenderTargetSetup(fbi->first)).c_str());
#endif

			// in order to avoid leaks when we destroy rb/tex that is still attached to some FBO
			// we need to
			// 1. attach 0 to all FBO points (buggy drivers)
			// 2. delete fbo itself
			m_Api.BindFramebuffer(gl::kDrawFramebuffer, fbi->second);
			const GLenum target = GetGraphicsCaps().gles.framebufferTargetForBindingAttachments;
			for(int i = 0, n = fbi->first.m_ColorCount ; i < n ; ++i)
				GLES_CALL(&m_Api, glFramebufferTexture2D, target, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);

			if (fbi->first.m_HasDepth)
			{
				GLES_CALL(&m_Api, glFramebufferRenderbuffer, target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
				GLES_CALL(&m_Api, glFramebufferRenderbuffer, target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}

			// It turns out that we found out that we are trying to delete the currently bound framebuffer so we can't rebind it after deleting it
			if (fbi->second == curFB)
				rebindCurrentFramebuffer = false;

			m_Api.DeleteFramebuffer(fbi->second, m_DefaultFBO);
			m_FramebufferMap.erase(fbi++);
		}
		else
		{
			++fbi;
		}
	}

	if (rebindCurrentFramebuffer)
		m_Api.BindFramebuffer(gl::kDrawFramebuffer, curFB);

	m_CurrentFramebufferSetup.fbo = m_Api.GetFramebufferBinding(gl::kDrawFramebuffer); // DeleteFramebuffer may change the current binding if current is deleted
	
	// update active FBOs

	m_PendingFramebufferValid = !HasAsAttachment(GLESRenderTargetSetup(m_PendingFramebuffer), rs);
	m_CurrentFramebufferValid = !HasAsAttachment(GLESRenderTargetSetup(m_CurrentFramebuffer), rs);
	if (!m_PendingFramebufferValid)
		ErrorString("RenderTexture warning: Destroying active render texture. Switching to main context.");

	FallbackToValidFramebufferState();

	AssertMsg(!InvalidateSurfacePtr(m_PendingFramebuffer, rs) && !InvalidateSurfacePtr(m_CurrentFramebuffer, rs),
		"GfxFramebufferGLES: An active RenderTargetSetup has dangling pointers.");

	// Effectively delete the rs object and delete the OpenGL renderbuffer of texture object it contains
	gles::DestroyRenderSurface(&m_Api, reinterpret_cast<RenderSurfaceGLES*>(rs));

	AssertMsg(m_Api.GetFramebufferBinding(gl::kDrawFramebuffer) == m_CurrentFramebufferSetup.fbo, "GfxFramebufferGLES: Inconsistent framebuffer setup");
}

void GfxFramebufferGLES::FallbackToValidFramebufferState()
{
	if (!m_CurrentFramebufferValid && !m_PendingFramebufferValid)
	{
		// both m_CurrentFramebuffer and m_PendingFramebuffer reference RS - just forcibly set default FBO
		this->MakeCurrentFramebuffer(GfxFramebufferGLES::kDefault);
		// Also activate after MakeCurrentFramebuffer to have a valid m_PendingFramebuffer
		// (Activate requires a valid m_CurrentFramebuffer, there might be nicer ways to achieve this...)
		this->Activate(GfxFramebufferGLES::kDefault);
	}
	else if (!m_PendingFramebufferValid)
	{
		// only m_PendingFramebuffer references RS - set default through usual mecanism
		this->Activate(GfxFramebufferGLES::kDefault);
	}
	else if (!m_CurrentFramebufferValid)
	{
		// only m_CurrentFramebuffer references RS (we set proper RT, but it was delayed)
		this->MakeCurrentFramebuffer(GfxFramebufferGLES::kPending);
	}
	Assert(m_CurrentFramebufferValid && m_PendingFramebufferValid);
}

void GfxFramebufferGLES::CleanupFBOMapForTextureID(const TextureID &tid)
{
	// We really shouldn't have this FBO as either the current or pending. Catch these if that happens.
	Assert(!::HasAsAttachment(GLESRenderTargetSetup(m_PendingFramebuffer), tid));
	Assert(!::HasAsAttachment(GLESRenderTargetSetup(m_CurrentFramebuffer), tid));
	for (FramebufferMap::iterator fbi = m_FramebufferMap.begin(), fbend = m_FramebufferMap.end(); fbi != fbend; )
	{
		if (::HasAsAttachment(fbi->first, tid))
		{

#if DEBUG_GLES_FRAMEBUFFER
			printf_console("*** GLES FBO Delete %d %s\n", GLES_OBJECT_NAME(fbi->second), DescribeRT(GLESRenderTargetSetup(fbi->first)).c_str());
#endif

			// in order to avoid leaks when we destroy rb/tex that is still attached to some FBO
			// we need to
			// 1. attach 0 to all FBO points (buggy drivers)
			// 2. delete fbo itself
			m_Api.BindFramebuffer(gl::kDrawFramebuffer, fbi->second);
			const GLenum target = GetGraphicsCaps().gles.framebufferTargetForBindingAttachments;
			for (int i = 0, n = fbi->first.m_ColorCount; i < n; ++i)
				GLES_CALL(&m_Api, glFramebufferTexture2D, target, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);

			if (fbi->first.m_HasDepth)
			{
				GLES_CALL(&m_Api, glFramebufferRenderbuffer, target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
				GLES_CALL(&m_Api, glFramebufferRenderbuffer, target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}

			m_Api.DeleteFramebuffer(fbi->second, m_DefaultFBO);
			m_FramebufferMap.erase(fbi++);
		}
		else
		{
			++fbi;
		}
	}
}

void GfxFramebufferGLES::CleanupFBOMapForRBID(const GLuint &rbid)
{
	// We really shouldn't have this FBO as either the current or pending. Catch these if that happens.
	DebugAssert(!::HasAsAttachment(GLESRenderTargetSetup(m_PendingFramebuffer), rbid));
	DebugAssert(!::HasAsAttachment(GLESRenderTargetSetup(m_CurrentFramebuffer), rbid));
	for (FramebufferMap::iterator fbi = m_FramebufferMap.begin(), fbend = m_FramebufferMap.end(); fbi != fbend; )
	{
		// There are corner cases where the fbo map gets surfaces bound to FBO 0. Just skip those for now. TODO:
		// figure out how to fix it properly without blowing everything up.
		if(fbi->second == 0)
		{
			fbi++;
			continue;
		}
		
		if (::HasAsAttachment(fbi->first, rbid))
		{

#if DEBUG_GLES_FRAMEBUFFER
			printf_console("*** GLES FBO Delete %d %s\n", GLES_OBJECT_NAME(fbi->second), DescribeRT(GLESRenderTargetSetup(fbi->first)).c_str());
#endif

			// in order to avoid leaks when we destroy rb/tex that is still attached to some FBO
			// we need to
			// 1. attach 0 to all FBO points (buggy drivers)
			// 2. delete fbo itself
			m_Api.BindFramebuffer(gl::kDrawFramebuffer, fbi->second);
			const GLenum target = GetGraphicsCaps().gles.framebufferTargetForBindingAttachments;
			for (int i = 0, n = fbi->first.m_ColorCount; i < n; ++i)
				GLES_CALL(&m_Api, glFramebufferTexture2D, target, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);

			if (fbi->first.m_HasDepth)
			{
				GLES_CALL(&m_Api, glFramebufferRenderbuffer, target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
				GLES_CALL(&m_Api, glFramebufferRenderbuffer, target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}

			m_Api.DeleteFramebuffer(fbi->second, m_DefaultFBO);
			m_FramebufferMap.erase(fbi++);
		}
		else
		{
			++fbi;
		}
	}
}


void GfxFramebufferGLES::DiscardContents(RenderSurfaceHandle& rs) const
{
	// discard only makes sense for current active rt
	bool isCurrent = false;
	if (rs.object->colorSurface)
	{
		for(int i = 0, n = m_PendingFramebuffer.colorCount ; i < n ; ++i)
			isCurrent = isCurrent || rs.object == m_PendingFramebuffer.color[i];
	}
	else
	{
		isCurrent = rs.object == m_PendingFramebuffer.depth;
	}

	rs.object->storeAction = (g_GraphicsCapsGLES->hasInvalidateFramebuffer && isCurrent) ? kGfxRTStoreActionDontCare : kGfxRTStoreActionStore;
	rs.object->loadAction = GetGraphicsCaps().hasTiledGPU ? kGfxRTLoadActionDontCare : kGfxRTLoadActionLoad;
}

void GfxFramebufferGLES::Invalidate()
{
	m_FramebufferMap.clear();

#if USES_GLES_DEFAULT_FBO
	m_DefaultGLESFBOInited = false;
	this->EnsureDefaultFBOInited();
#endif

	gles::UninitializeBlitFramebuffer(m_BlitQuad);
}

gles::BlitFramebufferDrawQuad& GfxFramebufferGLES::BlitQuad()
{
	gles::InitializeBlitFramebuffer(m_BlitQuad, gles::kBlitFramebufferSrcAlpha);
	return m_BlitQuad;
}

gl::FramebufferHandle GfxFramebufferGLES::GetFramebufferName(const GfxRenderTargetSetup& attach)
{
	GLESRenderTargetSetup glsetup(attach);
	FramebufferMap::iterator it = m_FramebufferMap.find(glsetup);
	if (it == m_FramebufferMap.end())
	{
		gl::FramebufferHandle framebufferName = ::CreateFramebuffer(*gGL, attach);
		it = m_FramebufferMap.insert(std::make_pair(glsetup, framebufferName)).first;
	}

	Assert(it != m_FramebufferMap.end());
	return it->second;
}

gl::FramebufferHandle GfxFramebufferGLES::GetFramebufferNameFromDepthAttachment(RenderSurfaceBase *depth)
{
	Assert(depth);
	GfxRenderTargetSetup attach;
	gles::internal::FillRenderTargetSetup(&attach, NULL, depth);
	return GetFramebufferName(attach);
}

gl::FramebufferHandle GfxFramebufferGLES::GetFramebufferNameFromColorAttachment(RenderSurfaceBase *color)
{
	Assert(color);
	GfxRenderTargetSetup attach;
	gles::internal::FillRenderTargetSetup(&attach, color, NULL);
	return GetFramebufferName(attach);
}

gl::FramebufferHandle GfxFramebufferGLES::GetCurrentFramebufferName()
{
	return m_CurrentFramebufferSetup.fbo;
}

gl::FramebufferHandle GfxFramebufferGLES::GetPendingFramebufferName()
{
	return m_PendingFramebufferSetup.fbo;
}

void GfxFramebufferGLES::RegisterExternalFBO(const GfxRenderTargetSetup & attach, gl::FramebufferHandle fbo)
{
	m_FramebufferMap[GLESRenderTargetSetup(attach)] = fbo;
}

void GfxFramebufferGLES::InvalidateActiveFramebufferState()
{
	// gles::Invalidate() calling this invalidates FBO state => needs rebind on next Prepare()
	m_RequiresFramebufferSetup = true;

	m_CurrentFramebufferSetup.fbo = m_Api.GetFramebufferBinding(gl::kDrawFramebuffer);
	m_CurrentFramebufferSetup.viewport.Reset();
	m_CurrentFramebufferSetup.scissor.Reset();
}

void GfxFramebufferGLES::Prepare()
{
	if (!m_RequiresFramebufferSetup)
	{
		Assert(m_CurrentFramebuffer.colorCount > 0 && m_CurrentFramebufferValid);
		return;
	}

	this->MakeCurrentFramebuffer(kPending);

	// Process all the pending mipmap gens from previous framebuffer (if any).
	static_cast<GfxDeviceGLES &>(GetRealGfxDevice()).ProcessPendingMipGens();

	bool avoidRestoreDepth = (m_CurrentFramebuffer.depth->loadAction == kGfxRTLoadActionDontCare);
	m_CurrentFramebuffer.depth->loadAction = kGfxRTLoadActionLoad;
	bool avoidRestoreColor[kMaxSupportedRenderTargets] = {};
	for (int i = 0; i < m_CurrentFramebuffer.colorCount; ++i)
	{
		RenderSurfaceGLES* curActiveColor = static_cast<RenderSurfaceGLES*>(m_CurrentFramebuffer.color[i]);
		avoidRestoreColor[i] = (curActiveColor->loadAction == kGfxRTLoadActionDontCare);
		curActiveColor->loadAction = kGfxRTLoadActionLoad;

		// If this surface needs mip gen, add it to the list of pending ones.
		if((curActiveColor->flags & kSurfaceCreateMipmap) && (curActiveColor->flags & kSurfaceCreateAutoGenMips))
			static_cast<GfxDeviceGLES &>(GetRealGfxDevice()).AddPendingMipGen(curActiveColor);
	}

	if (g_GraphicsCapsGLES->useDiscardToAvoidRestore)
	{
		this->InvalidateAttachments(avoidRestoreColor, avoidRestoreDepth);
	}

	if (g_GraphicsCapsGLES->useClearToAvoidRestore)
	{
		const ColorRGBAf clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gles::ClearCurrentFramebuffer(&m_Api, avoidRestoreColor[0], avoidRestoreDepth, avoidRestoreDepth, clearColor, 1.0f, 0);
	}

	m_RequiresFramebufferSetup = false;
}

void GfxFramebufferGLES::Clear(UInt32 clearFlags, const ColorRGBAf& color, float depth, int stencil, bool enforceClear)
{
	if (g_GraphicsCapsGLES->useClearToAvoidRestore && m_RequiresFramebufferSetup)
	{
		// if we will use clear to avoid restore in Prepare
		// we check if we have fullscreen clear and just drop avoidRestore on RenderSurfaces to avoid double clear

		const RectInt& vp = m_PendingFramebufferSetup.viewport;
		AssertMsg(m_PendingFramebuffer.colorCount > 0 && m_PendingFramebufferValid, "Clear called without valid active RenderTargetSetup");
		const RenderSurfaceGLES& surf = static_cast<RenderSurfaceGLES&>(*m_PendingFramebuffer.color[0]);

		const bool fullScreenClear = (vp.x == 0 && vp.y == 0 && vp.width == surf.width && vp.height == surf.height);
		if (fullScreenClear)
		{
			m_PendingFramebuffer.color[0]->loadAction = kGfxRTLoadActionLoad;
			m_PendingFramebuffer.depth->loadAction = kGfxRTLoadActionLoad;
		}
	}

	this->Prepare();

	const bool clearColor = gles::IsDummySurface(m_CurrentFramebuffer.color[0]) ? false : (clearFlags & kGfxClearColor) != 0;
	const bool clearDepth = gles::IsDummySurface(m_CurrentFramebuffer.depth) ? false : (clearFlags & kGfxClearDepth) != 0;
	bool clearStencil = gles::IsDummySurface(m_CurrentFramebuffer.depth) ? false : (clearFlags & kGfxClearStencil) != 0;

	// We can only do the check on non-backbuffer surfaces. For backbuffers, always assume we have stencil.
	if (clearStencil && !m_CurrentFramebuffer.depth->backBuffer)
	{
		if (!gl::IsFormatDepthStencil(((RenderSurfaceGLES *)m_CurrentFramebuffer.depth)->format))
			clearStencil = false;
	}

	DebugAssertMsg(m_Api.debug.FramebufferBindings(), "OPENGL ERROR: The OpenGL context has been modified outside of ApiGLES. States tracking is lost.");

	gles::ClearCurrentFramebuffer(&m_Api, clearColor, clearDepth, clearStencil, color, depth, stencil);
}

int GfxFramebufferGLES::GetSamplesCount() const
{
	return m_FramebufferSamplesCount;
}

void GfxFramebufferGLES::SetViewport(const RectInt& rect)
{
	m_PendingFramebufferSetup.viewport = rect;
	if (!m_RequiresFramebufferSetup)
		this->ApplyViewport();
}

void GfxFramebufferGLES::ApplyViewport()
{
	if (m_CurrentFramebufferSetup.viewport != m_PendingFramebufferSetup.viewport)
	{
		m_CurrentFramebufferSetup.viewport = m_PendingFramebufferSetup.viewport;
		GLES_CALL(&m_Api, glViewport, m_CurrentFramebufferSetup.viewport.x, m_CurrentFramebufferSetup.viewport.y, m_CurrentFramebufferSetup.viewport.width, m_CurrentFramebufferSetup.viewport.height);
	}
}

void GfxFramebufferGLES::SetScissor(const RectInt& rect)
{
	m_PendingFramebufferSetup.scissor = rect;
	if (!m_RequiresFramebufferSetup)
		this->ApplyScissor();
}

void GfxFramebufferGLES::ApplyScissor()
{
	if (m_CurrentFramebufferSetup.scissor != m_PendingFramebufferSetup.scissor)
	{
		m_CurrentFramebufferSetup.scissor = m_PendingFramebufferSetup.scissor;
		GLES_CALL(&m_Api, glScissor, m_CurrentFramebufferSetup.scissor.x, m_CurrentFramebufferSetup.scissor.y, m_CurrentFramebufferSetup.scissor.width, m_CurrentFramebufferSetup.scissor.height);
	}
}

void GfxFramebufferGLES::ActiveContextChanged(RenderSurfaceBase** outColor, RenderSurfaceBase** outDepth)
{
	this->SetupDefaultFramebuffer(outColor, outDepth, GetDefaultFBO());
	this->InvalidateActiveFramebufferState();	// force that framebuffer is rebound on next Prepare()
	this->FallbackToValidFramebufferState();	// make sure that current and pending framebuffer have valid rendertarget, may fall back to default framebuffer
	this->ProcessInvalidatedRenderSurfaces();
}

void GfxFramebufferGLES::AddRenderSurfaceToDeferredFBOInvalidateList(const RenderSurfaceGLES *rs)
{
	// rs is going to be deleted, remove from active rendertargets to avoid dangling pointers.
	// This may still leave e.g. m_CurrentFramebuffer in an invalid state (such as colorCounnt == 0).
	// This invalid state must be resolved explicitly using 'ActiveContextChanged'.
	m_PendingFramebufferValid = !InvalidateSurfacePtr(m_PendingFramebuffer, rs);
	m_CurrentFramebufferValid = !InvalidateSurfacePtr(m_CurrentFramebuffer, rs);

	if (rs->textureID.IsValid())
		m_InvalidatedTextureIDs.push_back(rs->textureID);
	if (rs->buffer != 0)
		m_InvalidatedRenderBufferIDs.push_back(rs->buffer);
	if (rs->stencilBuffer != 0)
		m_InvalidatedRenderBufferIDs.push_back(rs->stencilBuffer);
}

void GfxFramebufferGLES::ProcessInvalidatedRenderSurfaces()
{
	int i;
	for (i = 0; i < m_InvalidatedTextureIDs.size(); i++)
	{
		CleanupFBOMapForTextureID(m_InvalidatedTextureIDs[i]);
	}
	m_InvalidatedTextureIDs.clear();

	for (i = 0; i < m_InvalidatedRenderBufferIDs.size(); i++)
	{
		CleanupFBOMapForRBID(m_InvalidatedRenderBufferIDs[i]);
	}
	m_InvalidatedRenderBufferIDs.clear();
}


#if USES_GLES_DEFAULT_FBO

void GfxFramebufferGLES::EnsureDefaultFBOInited()
{
	if (!m_DefaultGLESFBOInited)
	{
		m_FramebufferMap[GLESRenderTargetSetup(m_DefaultFramebuffer)] =  m_DefaultFBO;
		if (m_DefaultFBO != gl::FramebufferHandle())
		{
			// Also add the system-default (0) FBO to the map, otherwise we're screwed afterwards.
			GLESRenderTargetSetup defSetup = GLESRenderTargetSetup::GetZeroSetup();
			m_FramebufferMap[defSetup] = gl::FramebufferHandle();
		}
		m_DefaultGLESFBOInited = true;
	}
}

#endif//USES_GLES_DEFAULT_FBO
