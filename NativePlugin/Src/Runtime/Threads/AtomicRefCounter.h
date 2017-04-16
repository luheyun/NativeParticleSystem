#ifndef __UNITY_ATOMIC_REFCOUNTER_H
#define __UNITY_ATOMIC_REFCOUNTER_H

#include "AtomicOps.h"

// Used for threadsafe refcounting
class AtomicRefCounter {
private:
	volatile int m_Counter;
public:
	// Upon the construction the self-counter is always set to 1,
	// which means that this instance is already accounted for.
	// This scheme shaves off the cycles that would be consumed
	// during the unavoidable first Retain call otherwise.
	AtomicRefCounter() : m_Counter(1) {}
	
	FORCE_INLINE void Retain ()
	{
		AtomicIncrement(&m_Counter);
	}

	FORCE_INLINE bool Release ()
	{
		int afterDecrement = AtomicDecrement(&m_Counter);
		Assert (afterDecrement >= 0); // If we hit this assert, someone is Releasing without matching it with Retain
		return afterDecrement == 0;
	}
	
	FORCE_INLINE int Count () const
	{
		return m_Counter;
	}

};

#endif // __UNITY_ATOMIC_REFCOUNTER_H
