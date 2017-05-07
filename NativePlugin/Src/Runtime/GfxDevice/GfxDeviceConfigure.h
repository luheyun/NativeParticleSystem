#pragma once

#include "Configuration/UnityConfigure.h"

#if UNITY_APPLE_PVR || UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN || UNITY_STV
#	define GFX_SUPPORTS_DXT_COMPRESSION 0
#	define GFX_SUPPORTS_DISPLAY_LISTS 0
#elif (UNITY_LINUX && GFX_SUPPORTS_OPENGL_UNIFIED)
// TODO Linux: determine the real configs here
#	define GFX_SUPPORTS_DXT_COMPRESSION 1
#	define GFX_SUPPORTS_DISPLAY_LISTS 1
#elif (UNITY_WIIU)
#	define GFX_SUPPORTS_DXT_COMPRESSION 1
#	define GFX_SUPPORTS_DISPLAY_LISTS 0
#else
#	define GFX_SUPPORTS_DXT_COMPRESSION 1
#	define GFX_SUPPORTS_DISPLAY_LISTS 1
#endif

#define GFX_HAS_TWO_EXTRA_TEXCOORDS UNITY_XENON

#ifndef GFX_ENABLE_DRAW_CALL_BATCHING
#define GFX_ENABLE_DRAW_CALL_BATCHING (UNITY_OSX || UNITY_WIN || UNITY_APPLE_PVR || UNITY_ANDROID || UNITY_LINUX || UNITY_PSP2 || UNITY_PS4 || UNITY_XENON || UNITY_BB10 || UNITY_WEBGL || UNITY_TIZEN || UNITY_STV || UNITY_XBOXONE)
#endif

// If creation of GPU programs is known to be thread safe then we can avoid a bit of overhead and reduce load on the GFX thread
// by doing the create in the same thread instead of passing it off to the GfxDeviceWorker and waiting for the result when parsing shaders.
#ifndef GFX_CREATE_GPU_PROGRAM_IS_THREAD_SAFE
#define GFX_CREATE_GPU_PROGRAM_IS_THREAD_SAFE UNITY_PSP2
#endif

// these platforms need to be able to recreate GpuPrograms, so we keep around the GpuProgram source data in memory.
// Note this only applies to standalone - the Editor is a special case as it retains source data to be able to reparse shaders.
// 2015-12-08 - only Windows Phone 8 used this
#ifndef GFX_CAN_RECREATE_GPU_PROGRAM
#define GFX_CAN_RECREATE_GPU_PROGRAM 0
#endif

#ifndef GFX_HAS_NO_FIXED_FUNCTION
#define GFX_HAS_NO_FIXED_FUNCTION (UNITY_PSP2 || UNITY_PS4 || UNITY_PS3 || UNITY_XENON || (GFX_SUPPORTS_OPENGL_UNIFIED && GFX_OPENGLESxx_ONLY))
#endif

#define GFX_SUPPORTS_DEFERRED_RENDER_LOOPS (!UNITY_BB10 && !UNITY_STV)

//@TODO: figure out if any of that buffer loss still happens on WebGL, BB, STV
#define GFX_ALL_BUFFERS_CAN_BECOME_LOST                      (GFX_SUPPORTS_OPENGL_UNIFIED && !UNITY_WIN && !UNITY_OSX && !UNITY_LINUX && !UNITY_APPLE_PVR && !UNITY_ANDROID && !UNITY_TIZEN)
#define GFX_BUFFERS_CAN_BECOME_LOST		GFX_SUPPORTS_D3D9 || (GFX_SUPPORTS_OPENGL_UNIFIED && !UNITY_WIN && !UNITY_OSX && !UNITY_LINUX && !UNITY_APPLE_PVR && !UNITY_ANDROID && !UNITY_TIZEN)

#define GFX_CAN_UNLOAD_MESH_DATA (!UNITY_EDITOR && !GFX_ALL_BUFFERS_CAN_BECOME_LOST)

#define GFX_SUPPORTS_SINGLE_PASS_STEREO (UNITY_PS4 || (GFX_SUPPORTS_D3D11 && !UNITY_WINRT))

#if !defined(GFX_SUPPORTS_CONSTANT_BUFFERS)
#define GFX_SUPPORTS_CONSTANT_BUFFERS (GFX_SUPPORTS_D3D11 || GFX_SUPPORTS_D3D12 || GFX_SUPPORTS_OPENGL_UNIFIED)
#endif

#define GFX_USE_SPHERE_FOR_POINT_LIGHT (!UNITY_PS3)

#ifndef GFX_HAS_OBJECT_LABEL
	#define GFX_HAS_OBJECT_LABEL (GFX_SUPPORTS_D3D11 || GFX_SUPPORTS_OPENGL_UNIFIED || GFX_SUPPORTS_METAL || (UNITY_XENON && !MASTER_BUILD))
#endif


// Enable to permit pure-threaded graphics device functionality
#define GFX_SUPPORTS_PURE_THREADING (UNITY_STANDALONE && GFX_SUPPORTS_D3D12)

// Enable to permit jobs in client/worker configurations 
// @MTTODO - Platform support needs to be feature define rather than the usual if UNITY_A or B or C ... or N
// Don't enable it on WinRT ARM devices as they devices don't really have enough cores to benefit from it today. Plus, it uses more power.
#ifndef GFX_SUPPORTS_THREADED_CLIENT_PROCESS
	#define GFX_SUPPORTS_THREADED_CLIENT_PROCESS (ENABLE_MULTITHREADED_CODE && (UNITY_STANDALONE || UNITY_XBOXONE || UNITY_PS4 || (UNITY_WINRT && !_M_ARM)))
#endif


// Jobified renderloops are supported.
// Currently users can enable / disable the jobified renderloop feature
// using the player settings graphicsJobs checkbox
// It results in that 
// * ShaderState::ApplyShaderState may be called on multiple jobs etc
// * Vertex declarations created from multiple jobs etc
#define GFX_SUPPORTS_JOBIFIED_RENDERLOOP (GFX_SUPPORTS_PURE_THREADING || GFX_SUPPORTS_THREADED_CLIENT_PROCESS)


// GPU Instancing: requires constant buffers, uniform array and instanced API support.
// Don't forget to update PlatformCanHaveInstancingVariant() when a new platform is added.
#define GFX_SUPPORTS_INSTANCING ((UNITY_WIN || UNITY_OSX || UNITY_LINUX /*|| UNITY_ANDROID || UNITY_IPHONE*/ || UNITY_PS4 || UNITY_XBOXONE) && GFX_SUPPORTS_CONSTANT_BUFFERS && GFX_ENABLE_DRAW_CALL_BATCHING)
