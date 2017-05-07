#include "PluginPrefix.h"
#include "Runtime/GfxDevice/opengles/GfxGetProcAddressGLES.h"
#include <cstddef>

#if ENABLE_EGL
	#include "Runtime/GfxDevice/egl/IncludesEGL.h"
#endif

#if UNITY_APPLE || UNITY_ANDROID || UNITY_STV
	#include <dlfcn.h>
#endif

#if UNITY_OSX
#import <mach-o/dyld.h>
#endif

#if UNITY_LINUX
#	include <GL/glx.h>
#endif

namespace gles
{
	void *GetProcAddress_core(const char *name)
	{
		void *proc = NULL;
		// TODO Add similar workarounds for other platforms that need it
#if UNITY_ANDROID
		static void* selfHandle = NULL;
		if(!selfHandle)
			selfHandle = dlopen("libGLESv2.so", RTLD_LOCAL | RTLD_NOW);
		proc = dlsym(selfHandle, name);
#else
		return GetProcAddress(name);
#endif

		return proc;
	}

	void* GetProcAddress(const char* name)
	{
		void* proc = NULL;

#if ENABLE_EGL
		proc = (void *)eglGetProcAddress(name);
#elif UNITY_WIN
		//proc = wglGetProcAddress(name);
		//if (!proc)
			proc = GetProcAddress(GetModuleHandle(L"libGLESv2.dll"), name);

#elif UNITY_LINUX
		proc = (void*)glXGetProcAddress((const GLubyte*)name);

#elif UNITY_APPLE
		// on apple platforms we link to framework, so symbols are already resolved
		static void* selfHandle = dlopen(0, RTLD_LOCAL | RTLD_LAZY);
		proc = dlsym(selfHandle, name);
#endif

		return proc;
	}

}//namespace gles
