#pragma once

#if UNITY_STV || UNITY_BB10 || UNITY_ANDROID || UNITY_WEBGL
#	include <KHR/khrplatform.h>
#elif UNITY_TIZEN
	typedef long				khronos_intptr_t;
	typedef long				khronos_ssize_t;
#elif UNITY_IOS || UNITY_TVOS || UNITY_OSX
	typedef intptr_t			khronos_intptr_t;
	typedef intptr_t			khronos_ssize_t;
#else
	typedef ptrdiff_t			khronos_intptr_t;
	typedef ptrdiff_t			khronos_ssize_t;
#endif

typedef void				GLvoid;
typedef unsigned int		GLenum;
typedef unsigned char		GLboolean;
typedef unsigned int		GLbitfield;
typedef signed char			GLbyte;
typedef short				GLshort;
typedef int					GLint;
typedef int					GLsizei;
typedef unsigned char		GLubyte;
typedef unsigned short		GLushort;
typedef unsigned int		GLuint;
typedef float				GLfloat;
typedef float				GLclampf;
typedef SInt32				GLfixed;
typedef char				GLchar;
typedef khronos_intptr_t	GLintptr;
typedef khronos_ssize_t		GLsizeiptr;
typedef unsigned short		GLhalf;
typedef SInt64				GLint64;
typedef UInt64				GLuint64;
typedef struct __GLsync*	GLsync;
typedef double				GLdouble;
