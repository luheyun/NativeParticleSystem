#include "PluginPrefix.h"

#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "DeviceStateGLES.h"
#include "AssertGLES.h"
//#include "GraphicsCapsGLES.h"

// TEMP: probably we should move c-interface into separate header
#include "GfxDeviceGLES.h"

#include "Runtime/Shaders/GraphicsCaps.h"

///////////////////////////////////////////////////////////////////////////////
//
// state reset
//

void gles::InvalidatePipelineStates(const GfxContextGLES & context, DeviceStateGLES& state)
{
	state.api->Invalidate(context);

	state.depthState	= state.depthStateNoDepthAccess;
	state.blendState	= gles::CreateBlendState(state, GfxBlendState());
	state.stencilState	= gles::CreateStencilState(state, GfxStencilState());
	state.rasterState	= gles::CreateRasterState(state, GfxRasterState());
}

void gles::Invalidate(const GfxContextGLES & context, DeviceStateGLES& state)
{
	state.stencilRefValue	= -1;

	state.appBackfaceMode = false;
	state.renderTargetsAreLinear = -1;

	InvalidatePipelineStates(context, state);
}

DeviceStateGLES* g_DeviceStateGLES	= 0;
