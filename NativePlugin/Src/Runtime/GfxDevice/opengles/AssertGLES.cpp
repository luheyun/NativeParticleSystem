#include "UnityPrefix.h"

#include "AssertGLES.h"
#include "GraphicsCapsGLES.h"
#include "ApiGLES.h"

#if !UNITY_RELEASE
#	define DUMP_CALL_STACK_ON_GLES_ERROR 0
#else
#	define DUMP_CALL_STACK_ON_GLES_ERROR 0
#endif

#if FORCE_DEBUG_BUILD_WEBGL
#	undef UNITY_WEBGL
#	define UNITY_WEBGL 1
#endif//FORCE_DEBUG_BUILD_WEBGL

#include <string>

namespace
{
	const GLenum GL_NO_ERROR						= 0;
	const GLenum GL_INVALID_ENUM					= 0x0500;
	const GLenum GL_INVALID_VALUE					= 0x0501;
	const GLenum GL_INVALID_OPERATION				= 0x0502;
	const GLenum GL_STACK_OVERFLOW					= 0x0503;
	const GLenum GL_STACK_UNDERFLOW					= 0x0504;
	const GLenum GL_OUT_OF_MEMORY					= 0x0505;
	const GLenum GL_INVALID_FRAMEBUFFER_OPERATION	= 0x0506;
}//namespace

namespace GLESDebug
{
	// Output the GL enum as a string to the output stream
	void PrintGLEnum(std::ostream &oss, GLenum en){}
}

static const char* GetErrorString(GLenum glerr)
{
	switch(glerr)
	{
	case GL_NO_ERROR:
		return "GL_NO_ERROR: No error occurred";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM: enum argument out of range";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE: Numeric argument out of range";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION: Operation illegal in current state";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY: Not enough memory left to execute command";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION: Framebuffer is not complete or incompatible with command";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW_KHR: OpenGL stack underflow";
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW_KHR: OpenGL stack overflow";
	default:
#if UNITY_WEBGL
		printf_console("AssertGles::GetErrorString invoked for unknown error %d",glerr);
#endif
		return "Unknown error";
	}
}

void LogGLES(const char *prefix, const char *message, const char* file, long line)
{
	std::string errorString(message);

	if (prefix)
		errorString = std::string(prefix) + ": " + errorString;

#	if DUMP_CALL_STACK_ON_GLES_ERROR
	DebugStringToFile(errorString.c_str(), 0, file, line, kAssert);
	DUMP_CALLSTACK(errorString.c_str());
#	else
	DebugStringToFile(errorString.c_str(), 0, file, line, kAssert);
#	endif
}

void CheckErrorGLES(const ApiGLES * const api, const char *prefix, const char* file, long line)
{
	Assert(api);

	const int kMaxErrors = 10;
	int counter = 0;

	if (!api->glGetError)
		return;
	GLenum glerr;
	while ((glerr = api->glGetError()) != GL_NO_ERROR)
	{
		LogGLES(prefix, GetErrorString(glerr), file, line);

		++counter;
		if (counter > kMaxErrors)
		{
			printf_console("GLES: error count exceeds %i, stop reporting errors\n", kMaxErrors);
			return;
		}
	}
}

void PreApiCallChecksGLES(const ApiGLES* api, const char* funcname, const char* file, long line)
{
	api->debug.LogApiCall(funcname, file, line);

	if (gl::GetCurrentContext())
		return;

	std::string errorString("no current context");
	if (funcname)
		errorString = std::string(funcname) + ": " + errorString;

	#if DUMP_CALL_STACK_ON_GLES_ERROR
		DebugStringToFile (errorString.c_str(), 0, file, line, kAssert);
		DUMP_CALLSTACK(errorString.c_str());
	#else
		DebugStringToFile (errorString.c_str(), 0, file, line, kAssert);
	#endif
}
