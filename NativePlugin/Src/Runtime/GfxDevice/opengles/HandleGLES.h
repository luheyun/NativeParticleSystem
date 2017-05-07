#pragma once

#include <cstddef>
#include "ApiTypeGLES.h"

#define UNITY_GLES_HANDLE_CONTEXT_CHECKS (!UNITY_RELEASE)

// Type-safe OpenGL handles and context checking in debug mode
//
// Handle types are defined near the bottom of this file using the GLES_DEFINE_SHARED_HANDLE(), GLES_DEFINE_PERCONTEXT_HANDLE() macros:
// e.g. GLES_DEFINE_PERCONTEXT_HANDLE(FramebufferHandle)
// GLES_DEFINE_PERCONTEXT_HANDLE() should be used for OpenGL handles that are not shared between contexts ('containers').
// GLES_DEFINE_SHARED_HANDLE() only provides type-safety. GLES_DEFINE_SHARED_HANDLE() can also be used to quickly disable context checks for certain handle types.
//
// To wrap a raw OpenGL name in a handle:
// gl::FramebufferHandle handle(name);
// This associates the OpenGL name with the current context and therefore it should only be used directly
// after the name was created (typically in ApiGLES.cpp)
//
// To access the raw OpenGL name, the macro GLES_OBJECT_NAME() should be used:
// gl::FramebufferHandle handle;
// GLint name = GLES_OBJECT_NAME(handle);
//
// Default-constructed handles are always zero-initialized and are valid in any context.
//
// In debug mode (UNITY_GLES_HANDLE_CONTEXT_CHECKS=1) context checks are performed when
// accessing the raw OpenGL name (GLES_OBJECT_NAME) or when using the (in-)equality operators.

namespace gl {

namespace detail {

// used when context checks are enabled, holds the context use for checks
class ObjectHandleBaseWithContext
{
public:
	ObjectHandleBaseWithContext()
	{
	}
};

// used when context checks are disabled, somewhat relying on empty-base opt.
class ObjectHandleBaseEmpty
{
public:
	ObjectHandleBaseEmpty()
	{
	}
};

struct NoErrorLoging
{
};

struct GetCurrentContextPolicyEnabled : NoErrorLoging
{
};

struct GetCurrentContextPolicyDisabled : NoErrorLoging
{
};

} // namespace detail

typedef detail::ObjectHandleBaseEmpty Shared;

#if UNITY_GLES_HANDLE_CONTEXT_CHECKS

	typedef detail::ObjectHandleBaseWithContext PerContext;
	typedef detail::GetCurrentContextPolicyEnabled DefaultGetCurrentContextPolicy;
	#define GLES_OBJECT_NAME(obj) (obj).Get(__FUNCTION__, __LINE__)
	#define GLES_OBJECT_CHECK_CONTEXT(obj) (obj).CheckContext(__FUNCTION__, __LINE__)

#else // UNITY_GLES_HANDLE_CONTEXT_CHECKS

	typedef detail::ObjectHandleBaseEmpty PerContext;
	typedef detail::GetCurrentContextPolicyDisabled DefaultGetCurrentContextPolicy;
	#define GLES_OBJECT_NAME(obj) (obj).Get()
	#define GLES_OBJECT_CHECK_CONTEXT(obj) do {} while(0)

#endif // UNITY_GLES_HANDLE_CONTEXT_CHECKS


// wraps a OpenGL object name and adds context information as debug info
template <typename Derived, typename ContextSharingPolicy, typename GetCurrentContextPolicy = DefaultGetCurrentContextPolicy>
class ObjectHandle : protected ContextSharingPolicy, protected GetCurrentContextPolicy
{
	// safe bool helpers
	struct SafeBool { int SafeTrue; };
	typedef int (SafeBool::*BoolType);
public:
	// constant that defines an invalid value (not 0).
	static const Derived kInvalidValue;

	ObjectHandle()
	: ContextSharingPolicy()
	, GetCurrentContextPolicy()
	, m_Name(0)
	{
	}

	explicit ObjectHandle(GLuint name)
	: ContextSharingPolicy()
	, GetCurrentContextPolicy()
	, m_Name(name)
	{
	}

	// create a default constructed 0-handle, helper to avoid "most vexing parse"
	static Derived Zero()
	{
		return Derived();
	}

	friend bool operator==(const Derived& lhs, const Derived& rhs)
	{
        bool valid = lhs.m_Name == 0 || rhs.m_Name == 0 ||										// m_Name == 0 is considered valid in all contexts.
            lhs.m_Name == kInvalidValue.m_Name || rhs.m_Name == kInvalidValue.m_Name;	// same for kInvalidValue

		return lhs.m_Name == rhs.m_Name;
	}

	friend bool operator!=(const Derived& lhs, const Derived& rhs)
	{
		return !(lhs == rhs);
	}

	// returns the raw OpenGL name
	GLuint Get() const
	{
		return m_Name;
	}

	GLuint Get(const char* function, unsigned int line) const
	{
		return m_Name;
	}

	GLuint GetUnchecked() const
	{
		return m_Name;
	}

	// safe-bool to avoid implcit conversion to int
	operator BoolType() const
	{
		return m_Name ? static_cast<BoolType>(&SafeBool::SafeTrue) : NULL;
	}

protected:
	// protected - don't allow sliced copies
	~ObjectHandle()
	{
	}

private:
	GLuint m_Name;
};

namespace detail {

template <typename HandleType>
HandleType MakeInvalidValue()
{
	struct InvalidHandle : HandleType
	{
		InvalidHandle()	: HandleType(0xffffffff)
		{
		}
	};

	return InvalidHandle();
}

} // namespace detail

template <typename Derived, typename ContextSharingPolicy, typename GetCurrentContextPolicy>
const Derived ObjectHandle<Derived, ContextSharingPolicy, GetCurrentContextPolicy>::kInvalidValue(detail::MakeInvalidValue<Derived>());

#define GLES_DEFINE_HANDLE_INTERNAL(TypeName, SharingPolicy, CurrentContext)								\
	struct TypeName : ObjectHandle<TypeName, SharingPolicy, CurrentContext>									\
	{																										\
		explicit TypeName(GLuint name = 0) : ObjectHandle<TypeName, SharingPolicy, CurrentContext>(name) {}	\
		static const char* Name() { return #TypeName; }														\
	}

#define GLES_DEFINE_SHARED_HANDLE(TypeName) GLES_DEFINE_HANDLE_INTERNAL(TypeName, Shared, DefaultGetCurrentContextPolicy)
#define GLES_DEFINE_PERCONTEXT_HANDLE(TypeName) GLES_DEFINE_HANDLE_INTERNAL(TypeName, PerContext, DefaultGetCurrentContextPolicy)

// define type-safe object handles here
GLES_DEFINE_PERCONTEXT_HANDLE(FramebufferHandle);
GLES_DEFINE_PERCONTEXT_HANDLE(VertexArrayHandle);
GLES_DEFINE_PERCONTEXT_HANDLE(QueryHandle);

#undef GLES_DEFINE_SHARED_HANDLE
#undef GLES_DEFINE_PERCONTEXT_HANDLE

} // namespace gl

#undef UNITY_GLES_HANDLE_CONTEXT_CHECKS
