#pragma once

#include "ApiGLES.h"
#include "Runtime/Threads/Thread.h"

// To enable OpenGL asserts, set ENABLE_GL_DEBUG_ASSERT to 1
#define ENABLE_GL_DEBUG_ASSERT 0

// Debug only helper: to run OpenGL asserts on Katana: replace with #if 1.
#define DEBUG_GL_ERROR_CHECKS (!UNITY_RELEASE && ENABLE_GL_DEBUG_ASSERT)

class ApiGLES;
void CheckErrorGLES(const ApiGLES * const, const char *prefix, const char* file, long line);
void PreApiCallChecksGLES(const ApiGLES * const, const char *funcname, const char* file, long line);

#if defined(_MSC_VER)
#	define GLES_FUNCTION __FUNCTION__
#elif defined(__clang__) || defined(__GNUC__)
#	if __cplusplus >= 201103L
#		define GLES_FUNCTION __func__
#	else
#		define GLES_FUNCTION __FUNCTION__
#	endif
#else
#	define GLES_FUNCTION
#endif

// Debug only helper: to run OpenGL asserts on Katana: replace with #if 1.
//#if !UNITY_RELEASE
//	// Check that OpenGL calls are made on the correct thread
//	#define GLES_CHECK(api, name)						do {(api)->Check(name, GLES_FUNCTION, __LINE__);} while(0)
//
//	// Normal AssertMsg followed by glGetError but only in debug build. These checks are completely discarded in release.
//	//#define GLES_ASSERT(api, x, message)				do { AssertMsg(x, (std::string("OPENGL ERROR: ") + message).c_str()); {CheckErrorGLES((api), "OPENGL ERROR", __FILE__, __LINE__);}} while(0)
//
//	// Call OpenGL functions with full checking
//	#define GLES_CALL(api, funcname, ...)				do { PreApiCallChecksGLES(api, #funcname, __FILE__, __LINE__); Assert((api)->funcname); {(api)->funcname(__VA_ARGS__);} {CheckErrorGLES((api), #funcname, __FILE__, __LINE__);} } while(0)
//
//	// Call OpenGL functions that return a value with full checking
//	#define GLES_CALL_RET(api, retval, funcname, ...)	do { PreApiCallChecksGLES(api, #funcname, __FILE__, __LINE__); Assert((api)->funcname); {retval = (api)->funcname(__VA_ARGS__);} {CheckErrorGLES((api), #funcname, __FILE__, __LINE__);} } while(0)
//#else
	#define GLES_CHECK(api, name)
	#define GLES_ASSERT(api, x, message)

	#define GLES_CALL(api, funcname, ...)				(api)->funcname(__VA_ARGS__)
	#define GLES_CALL_RET(api, retval, funcname, ...)	retval = (api)->funcname(__VA_ARGS__)
//#endif

#define GLES_PRINT_GL_TRACE 0
#define GLES_DUMP_CALLSTACK_WITH_TRACE 0
#define DBG_LOG_GLES_ACTIVE 0
#define DBG_SHADER_VERBOSE_ACTIVE 0

#if GLES_PRINT_GL_TRACE

#include <sstream>

namespace GLESDebug
{
	// Output the GL enum as a string to the output stream
	void PrintGLEnum(std::ostream &oss, GLenum en);
}

// Empty version for handling the no-arguments case
static inline void gl_print_args(std::ostream &out) {}

template<typename A> static inline void gl_print_args(std::ostream &out, const A &a) { out << a; }
//template<typename A, typename... Rest> static inline void gl_print_args(std::ostream &out, const A &a, Rest... rest) { gl_print_args(out, a); out << ", "; gl_print_args(out, rest); }

// Partial specializations for pointer types and anything that might be GLenum (C++ won't treat typedefs as proper types)
// GL enum handlers are currently disabled after unified GL work. TODO: re-enable.
//static inline void gl_print_args(std::ostream &out, const GLenum &a) { GLESDebug::PrintGLEnum(out, (GLenum)a); }
//static inline void gl_print_args(std::ostream &out, const int &a) { GLESDebug::PrintGLEnum(out, (GLenum)a); }

// Char printer, add the "'s, limit to 20 chars to prevent garbage in output
static inline void gl_print_args(std::ostream &out, const char * const &a)
{
	if (strnlen(a, 20) == 20)
	{
		char temp[20];
		strncpy(temp, a, 19);
		temp[19] = 0;
		out << "\"" << temp << "\"";
	}
	else
		out << "\"" << a << "\"";
}

// texture data is given as UInt8, so don't try to print that out. May cause strings to not show up on GCC.
template <typename T> static inline void gl_print_args(std::ostream &out, T * const &a) { if(a) out << "<addr>"; else out << "null"; }


// Would be a lot neater with variadic templates but that's c++11 only.
template<typename A, typename B> static inline void gl_print_args(std::ostream &out, const A &a, const B &b) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); }
template<typename A, typename B, typename C> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); }
template<typename A, typename B, typename C, typename D> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); }
template<typename A, typename B, typename C, typename D, typename E> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); }
template<typename A, typename B, typename C, typename D, typename E, typename F> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); }
template<typename A, typename B, typename C, typename D, typename E, typename F, typename G> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f, const G &g) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); out << ", "; gl_print_args(out, g); }
template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f, const G &g, const H &h) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); out << ", "; gl_print_args(out, g); out << ", "; gl_print_args(out, h); }
template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f, const G &g, const H &h, const I &i) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); out << ", "; gl_print_args(out, g); out << ", "; gl_print_args(out, h); out << ", "; gl_print_args(out, i); }
template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f, const G &g, const H &h, const I &i, const J &j) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); out << ", "; gl_print_args(out, g); out << ", "; gl_print_args(out, h); out << ", "; gl_print_args(out, i); out << ", "; gl_print_args(out, j); }
template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f, const G &g, const H &h, const I &i, const J &j, const K &k) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); out << ", "; gl_print_args(out, g); out << ", "; gl_print_args(out, h); out << ", "; gl_print_args(out, i); out << ", "; gl_print_args(out, j); out << ", "; gl_print_args(out, k); }
template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K, typename L> static inline void gl_print_args(std::ostream &out, const A &a, const B &b, const C &c, const D &d, const E &e, const F &f, const G &g, const H &h, const I &i, const J &j, const K &k, const L &l) { gl_print_args(out, a); out << ", "; gl_print_args(out, b); out << ", "; gl_print_args(out, c); out << ", "; gl_print_args(out, d); out << ", "; gl_print_args(out, e); out << ", "; gl_print_args(out, f); out << ", "; gl_print_args(out, g); out << ", "; gl_print_args(out, h); out << ", "; gl_print_args(out, i); out << ", "; gl_print_args(out, j); out << ", "; gl_print_args(out, k); out << ", "; gl_print_args(out, l); }

#if GLES_DUMP_CALLSTACK_WITH_TRACE
#define GLES_CALLSTACK() DUMP_CALLSTACK("GLESTrace: ")
#else
#define GLES_CALLSTACK()
#endif
#undef GLES_CALL
#undef GLES_CALL_RET
#define GLESCHKSTRINGIFY(x) GLESCHKSTRINGIFY2(x)
#define GLESCHKSTRINGIFY2(x) #x
#ifdef _MSC_VER
#define GLES_CALL(api, funcname, ...)	do { { std::ostringstream oss; gl_print_args(oss, __VA_ARGS__); printf_console("GLES: %s(%s) %s:%d\n", GLESCHKSTRINGIFY(funcname), oss.str().c_str(), __FILE__, __LINE__); (api)->funcname(__VA_ARGS__); } GLESAssert(api); GLES_CALLSTACK();} while(0)
#define GLES_CALL_RET(api, retval, funcname, ...)	do { { std::ostringstream oss; gl_print_args(oss, __VA_ARGS__); printf_console("GLES: %s = %s(%s) %s:%d\n", GLESCHKSTRINGIFY(retval), GLESCHKSTRINGIFY(funcname), oss.str().c_str(), __FILE__, __LINE__); retval = (api)->funcname(__VA_ARGS__); } GLESAssert(api);  GLES_CALLSTACK();} while(0)
#else
#define GLES_CALL(api, funcname, ...)	do { { std::ostringstream oss; gl_print_args(oss, ##__VA_ARGS__); printf_console("GLES: %s(%s) %s:%d\n", GLESCHKSTRINGIFY(funcname), oss.str().c_str(), __FILE__, __LINE__); (api)->funcname(__VA_ARGS__); } GLESAssert(api);  GLES_CALLSTACK();} while(0)
#define GLES_CALL_RET(api, retval, funcname, ...)	do { { std::ostringstream oss; gl_print_args(oss, ##__VA_ARGS__); printf_console("GLES: %s = %s(%s) %s:%d\n", GLESCHKSTRINGIFY(retval), GLESCHKSTRINGIFY(funcname), oss.str().c_str(), __FILE__, __LINE__); retval = (api)->funcname(__VA_ARGS__); } GLESAssert(api);  GLES_CALLSTACK();} while(0)
#endif

#endif

#if DBG_LOG_GLES_ACTIVE
#	define DBG_LOG_GLES(...) {printf_console(__VA_ARGS__);printf_console("\n");}
#else
#	define DBG_LOG_GLES(...)
#endif

#if DBG_SHADER_VERBOSE_ACTIVE
#define DBG_SHADER_VERBOSE_GLES DBG_LOG_GLES
#else
#define DBG_SHADER_VERBOSE_GLES(...)
#endif
