#pragma once

#include "ApiGLES.h"
//#include "RenderSurfaceGLES.h"
//#include "BlitFramebufferGLES.h"
//#include "Runtime/GfxDevice/GfxDeviceObjects.h"
#include "Runtime/GfxDevice/GfxDeviceTypes.h"
#include "Runtime/Math/Rect.h"
#include "Runtime/Math/Color.h"
#include "Runtime/Utilities/NonCopyable.h"

#ifndef USES_GLES_DEFAULT_FBO
	#define USES_GLES_DEFAULT_FBO !(UNITY_APPLE_PVR || UNITY_ANDROID || UNITY_TIZEN)
#endif

typedef unsigned int HandleGLES;

class ImageReference;
class GfxContextGLES;

class GfxFramebufferGLES : NonCopyable
{
public:
	GfxFramebufferGLES(ApiGLES & api, void* context);

public:
	enum Type
	{
		kBack, // Default framebuffer
		kObject // Framebuffer object
	};

	enum Builtin
	{
		kDefault, // Default framebuffer / backbuffer
		kPending, // Delayed framebuffer
		kCurrent // Current openGL state frmaebuffer
	};

	gl::FramebufferHandle GetCurrentFramebufferName();
	gl::FramebufferHandle GetPendingFramebufferName();

	// Return the number of samples of the active multisample framebuffer
	// Return 1 if not multisampled
	int GetSamplesCount() const;

	// Clear the framebuffer object map, called when shutting down
	void Invalidate();

	// Select one of the builtin framebuffer: kDefault, kPending, kCurrent
	void Activate(Builtin builtin, bool clear = false);

	// forcibly sets fbo and along with viewport/scissor if needed
	void MakeCurrentFramebuffer(Builtin builtin);

	// Honestly, just random mess that we try to avoid to do with m_NeedFramebufferSetup
	//
	// Take care of setting up fbo/setup if needed
	// Clear is separated to avoid double clear but on tiled gpu we automatically
	// clear the framebuffer anyway to avoid restauring the framebuffer on on-chip memeory from graphics memory
	void Prepare();

	// Effectively clear the bound framebuffer
	void Clear(UInt32 clearFlags, const ColorRGBAf& color, float depth, int stencil, bool enforceClear);

	// Invalidate the depth buffer of the default framebuffer
	// This will only actually invalidate depth if the default framebuffer is already the current framebuffer,
	// otherwise nothing is done
	void TryInvalidateDefaultFramebufferDepth();

	// will update backbuffer from window extents: it is needed in cases where we switch gl context but still drawing to default fbo
	void UpdateDefaultFramebufferViewport();

	// reestablish consistency in state tracking
	void InvalidateActiveFramebufferState();


	// Update the pending scissor. Update the current scissor only if internal states demands it
	void SetScissor(const RectInt& rect);

	// Update the pending viewport. Update the current viewport only if internal states demands it
	void SetViewport(const RectInt& rect);

	gl::FramebufferHandle GetDefaultFBO() const { return m_DefaultFBO; }

	// Notify the GfxFramebufferGLES that given (usually externally created) texture or renderbuffer id has been deleted so it should be scrubbed from the FBO map.
	void CleanupFBOMapForRBID(const GLuint &rbid);


private:
	// Update the current scissor from the pending scissor: it checks if the OpenGL states need to be
	void ApplyScissor();

	// Update the current viewport from the pending viewport: it checks if the OpenGL states need to be
	void ApplyViewport();


	// Called once context is activated. Processes the list generated with calls to the above, and cleans up FBOs
	void ProcessInvalidatedRenderSurfaces();

	// Restore a valid state of current and pending framebuffer if one or both of them are in an invalid state.
	// This may cause a switch to the default framebuffer. 
	void FallbackToValidFramebufferState();

	// Invalidate a specific set of framebuffer attachments
	void InvalidateAttachments(const bool discardColor[kMaxSupportedRenderTargets], bool discardDepth);

	// Invalidate all the framebuffer attachments
	void InvalidateAttachments();

private:
	// we dont have an explicit link RT->FBO, because we started out with shared FBO + reattach
	// on the other hand on tiled GPUs FBO is more then just attachments (tiler setup etc)
	// so we really want to have FBO per RT
	// the easiest way would be to have a map hidden in gles code
	int								m_FramebufferSamplesCount;

	bool							m_CurrentFramebufferValid;

	// Pending framebuffer for delayed framebuffer setup
	bool							m_PendingFramebufferValid;

	// The flag to avoid redundant and to delay glViewport and glScissor calls
	bool							m_RequiresFramebufferSetup;

	ApiGLES &						m_Api;


	gl::FramebufferHandle			m_DefaultFBO; // Editor game view (and other places) may override the back buffer, and FBOs are per context.

	// Arrays of texture ids and renderbuffer ids that have been deleted since this context was last active.
	// Clean up the FBO map of anything that contains these.
	dynamic_array<GLuint>			m_InvalidatedRenderBufferIDs;

#if USES_GLES_DEFAULT_FBO
	bool							m_DefaultGLESFBOInited;
	void EnsureDefaultFBOInited();
#endif
};
