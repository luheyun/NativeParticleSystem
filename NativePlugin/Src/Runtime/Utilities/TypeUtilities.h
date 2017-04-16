// Utility functions and types for working with static types.
#pragma once

template<typename T1, typename T2>
struct IsSameType
{
	static const bool result = false;
};

template<typename T>
struct IsSameType<T, T>
{
	static const bool result = true;
};

template<typename Expected, typename Actual>
inline bool IsOfType (Actual&)
{
	return IsSameType<Actual, Expected>::result;
}


template<typename T>
struct ToPointerType
{
	typedef T* ResultType;
};
template<typename T>
struct ToPointerType<T*>
{
	typedef T* ResultType;
};
template<typename T>
struct ToPointerType<const T*>
{
	typedef const T* ResultType;
};


//////////////////////////////////////////////////////////////////////////
/// std::is_base_of equievalent
/// See http://stackoverflow.com/questions/2910979/how-does-is-base-of-work
template <typename B, typename D>
class IsBaseOfType
{
	typedef struct { char m; } yes;
	typedef struct { char m[2]; } no;

	template <typename E, typename F>
	struct Host
	{
		operator E*() const;
		operator F*();
	};

	template <typename T>
	static yes deduce(D*, T);
	static no deduce(B*, int);

public:
	static const bool result = sizeof(deduce(Host<B, D>(), int())) == sizeof(yes);
};

//////////////////////////////////////////////////////////////////////////
/// std::conditional equievalent
template<bool B, class T, class F>
struct Conditional { typedef T type; };

template<class T, class F>
struct Conditional<false, T, F> { typedef F type; };

//////////////////////////////////////////////////////////////////////////
/// std::enable_if equievalent
template<bool B, class T = void>
struct EnableIf {};

template<class T>
struct EnableIf < true, T > { typedef T type; };

//////////////////////////////////////////////////////////////////////////
/// Empty type
struct NullType {};

//////////////////////////////////////////////////////////////////////////
/// Default value generation
template<typename T>
struct DefaultValue 
{
	static T Get() { return T(); }
};

template<>
struct DefaultValue<void> 
{
	static void Get() { }
};