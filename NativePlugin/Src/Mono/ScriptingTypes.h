//////////////////////////////////////////////////////////////
//     FileName:    ScriptingTypes.h 
//     Author:      taoxu
//     Date:        2017-2-14 
//     Description: 脚本类型定义
/////////////////////////////////////////////////////////////
#pragma once

#ifndef _MSC_VER
#include <stdint.h>
#endif

typedef int            gboolean;
typedef int            gint;
typedef unsigned int   guint;
typedef short          gshort;
typedef unsigned short gushort;
typedef long           glong;
typedef unsigned long  gulong;
typedef void *         gpointer;
typedef const void *   gconstpointer;
typedef char           gchar;
typedef unsigned char  guchar;

#ifdef _MSC_VER
typedef __int8				gint8;
typedef unsigned __int8		guint8;
typedef __int16				gint16;
typedef unsigned __int16	guint16;
typedef __int32				gint32;
typedef unsigned __int32	guint32;
typedef __int64				gint64;
typedef unsigned __int64	guint64;
typedef float				gfloat;
typedef double				gdouble;
typedef unsigned __int16	gunichar2;
#else
/* Types defined in terms of the stdint.h */
typedef int8_t         gint8;
typedef uint8_t        guint8;
typedef int16_t        gint16;
typedef uint16_t       guint16;
typedef int32_t        gint32;
typedef uint32_t       guint32;
typedef int64_t        gint64;
typedef uint64_t       guint64;
typedef float          gfloat;
typedef double         gdouble;
typedef uint16_t       gunichar2;
#endif

#ifdef USE_MONO

struct MonoObject;
struct MonoString;
struct MonoArray;
struct MonoClass;
struct MonoException;
struct MonoImage;
struct MonoDomain;
struct MonoAssembly;
struct MonoMethod;
struct MonoClassField;

typedef MonoObject ScriptingObject;
typedef MonoString ScriptingString;
typedef MonoArray ScriptingArray;
typedef MonoClass ScriptingClass;
typedef MonoException ScriptingException;
typedef MonoImage ScriptingImage;
typedef MonoDomain ScriptingDomain;
typedef MonoAssembly ScriptingAssembly;
typedef MonoMethod ScriptingMethod;
typedef MonoClassField ScriptingField;

#elif defined(USE_IL2CPP)

struct TypeInfo;
struct Il2CppType;
struct EventInfo;
struct MethodInfo;
struct FieldInfo;
struct PropertyInfo;

struct Il2CppAssembly;
struct Il2CppArray;
struct Il2CppDelegate;
struct Il2CppDomain;
struct Il2CppImage;
struct Il2CppException;
struct Il2CppProfiler;
struct Il2CppObject;
struct Il2CppReflectionMethod;
struct Il2CppReflectionType;
struct Il2CppString;
struct Il2CppThread;
struct Il2CppAsyncResult;

typedef Il2CppObject ScriptingObject;
typedef Il2CppString ScriptingString;
typedef Il2CppArray ScriptingArray;
typedef TypeInfo ScriptingClass;
typedef Il2CppException ScriptingException;
typedef Il2CppImage ScriptingImage;
typedef Il2CppDomain ScriptingDomain;
typedef Il2CppAssembly ScriptingAssembly;
typedef MethodInfo ScriptMethod;
typedef FieldInfo ScriptingField;
typedef MethodInfo ScriptingMethod;
#endif

typedef unsigned char ScriptingBool;

typedef ScriptingObject* ScriptingObjectPtr;
typedef ScriptingString* ScriptingStringPtr;
typedef ScriptingArray* ScriptingArrayPtr;
typedef ScriptingClass* ScriptingClassPtr;
typedef ScriptingException* ScriptingExceptionPtr;
typedef ScriptingObjectPtr ScriptingSystemTypeObjectPtr;
typedef void* ScriptingParams;
typedef ScriptingParams* ScriptingParamsPtr;
typedef ScriptingImage* ScriptingImagePtr;
typedef ScriptingDomain* ScriptingDomainPtr;
typedef unsigned char ScriptingBool;

struct UnityEngineObjectMemoryLayout
{
	int         instanceID;
	void*       cachedPtr;
};
//Unity messes around deep in mono internals, bypassing mono's API to do certain things. One of those things is we have c++
//structs that we assume are identical to c# structs, and when a mono method calls into the unity runtime, we make the assumption
//that we can access the data stored in the c# struct directly.  When a MonoObject* gets passed to us by mono, and we know that the
//object it represents is a struct, we take the MonoObject*,  add 8 bytes, and assume the data of the c# struct is stored there in memory.
//When you update to a newer version of mono, and get weird crashes, the assumptions that these offsets make are a good one to verify.
enum { kMonoObjectOffset = sizeof(void*)* 2 };
enum { kMonoArrayOffset = sizeof(void*)* 4 };

struct LogicObjectMemoryLayout
{
	void*       cachedPtr;
};




