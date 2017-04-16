#ifndef THREADSPECIFICVALUE_H
#define THREADSPECIFICVALUE_H

#include "Configuration/UnityConfigure.h"

#if !SUPPORT_THREADS
	class PlatformThreadSpecificValue
	{
	public:
		PlatformThreadSpecificValue();
		~PlatformThreadSpecificValue();

		void* GetValue() const;
		void SetValue(void* value);

	private:
		void*	m_Value;
	};

	inline PlatformThreadSpecificValue::PlatformThreadSpecificValue()
	{
		m_Value = 0;
	}

	inline PlatformThreadSpecificValue::~PlatformThreadSpecificValue()
	{
		//do nothing
	}

	inline void* PlatformThreadSpecificValue::GetValue() const
	{
		return m_Value;
	}

	inline void PlatformThreadSpecificValue::SetValue(void* value)
	{
		m_Value = value;
	}
#elif UNITY_WIN
#	include "Winapi/PlatformThreadSpecificValue.h"
#elif UNITY_POSIX
#	include "Posix/PlatformThreadSpecificValue.h"
#else
#	include "PlatformThreadSpecificValue.h"
#endif

#if UNITY_DYNAMIC_TLS

template<class T> class ThreadSpecificValue
{
	PlatformThreadSpecificValue m_TLSKey;

public:
	inline operator T () const
	{
		return (T) (UIntPtr) GetValue ();
	}

	inline T operator -> () const
	{
		return (T) (UIntPtr) GetValue ();
	}

	inline T operator ++ ()
	{
		*this = *this + 1;
		return *this;
	}

	inline T operator -- ()
	{
		*this = *this - 1;
		return *this;
	}

	inline T operator = (T value)
	{
		SetValue ((void*) (UIntPtr) value);
		return value;
	}

private:

	void* GetValue() const;
	void SetValue(void* value);
};

template<class T> void* ThreadSpecificValue<T>::GetValue () const
{
	return m_TLSKey.GetValue();
}

template <class T> void ThreadSpecificValue<T>::SetValue (void* value)
{
	m_TLSKey.SetValue(value);
}

#define UNITY_TLS_VALUE(type) ThreadSpecificValue<type>

#else

#	ifndef UNITY_TLS_VALUE
#		error "A static TLS mechanism is not defined for this platform"
#	endif

#endif // UNITY_DYNAMIC_TLS

#endif
