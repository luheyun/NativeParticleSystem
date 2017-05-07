#pragma once

#include "Runtime/Utilities/dynamic_array.h"
#include "Runtime/GfxDevice/TransformState.h"
//#include "Runtime/Math/Rect.h"
//#include "ConstantBuffersGLES.h"
//#include "GfxDeviceResourcesGLES.h"

class ApiGLES;
class UniformCacheGLES;

namespace gles
{
    //typedef std::set<DeviceDepthStateGLES, memcmp_less<DeviceDepthStateGLES> > CachedDepthStates;
    //typedef std::set<DeviceBlendStateGLES, memcmp_less<DeviceBlendStateGLES, DeviceBlendState> > CachedBlendStates;
    //typedef std::set<DeviceStencilStateGLES, memcmp_less<DeviceStencilStateGLES, DeviceStencilState> > CachedStencilStates;
    //typedef std::set<DeviceRasterStateGLES, memcmp_less<DeviceRasterStateGLES> > CachedRasterStates;
}

struct DeviceStateGLES
{
    ApiGLES*						api;

    UInt32							transformDirtyFlags;


    UniformCacheGLES*								activeUniformCache;

    //const DeviceDepthStateGLES*		depthState;
    //const DeviceStencilStateGLES*	stencilState;
    //const DeviceBlendStateGLES*		blendState;
    //const DeviceRasterStateGLES*	rasterState;

    //gles::CachedDepthStates			depthStateCache;
    //gles::CachedStencilStates		stencilStateCache;
    //gles::CachedBlendStates			blendStateCache;
    //gles::CachedRasterStates		rasterStateCache;

    // no depth access (no attachment, or simply disabled)
    //const DeviceDepthStateGLES*		depthStateNoDepthAccess;

    //// colorMask = 0, no blending, no alpha test
    //const DeviceBlendStateGLES*		blendStateNoColorNoAlphaTest;

    //RectInt							viewport;
    //RectInt							scissorRect;

    int								stencilRefValue;

    int								renderTargetsAreLinear; // 0/1 or -1

    int								randomWriteTargetMaxIndex;

    BarrierTime						barrierTimes[gl::kBarrierTypeCount]; // Record the times of last called barriers
    BarrierTime						barrierTimeCounter; // Time counter for barrier resolving. Incremented on each write/barrier
    GLbitfield						requiredBarriers; // Bitfield marking the required barriers before next draw/dispatch call
    GLbitfield						requiredBarriersMask; // Mask for temporarily enabling only a subset of barriers
};

// TODO: this file gets pretty big (though cannot be really avoided on the move)
// most of this should be part of common GfxDevice, and others should probably be moved around a bit to avoid clutter

namespace gles
{
    // state reset
    void	Invalidate(const GfxContextGLES & context, DeviceStateGLES& state);
    void	InvalidatePipelineStates(const GfxContextGLES & context, DeviceStateGLES& state);
}


// TODO: might be not the best place
// TODO: get rid of global state?
extern DeviceStateGLES* g_DeviceStateGLES;
