#pragma once

#include "Threads/ExtendedAtomicTypes.h"

class AtomicNode
{
	friend class AtomicStack;
	friend class AtomicQueue;
	friend class MutexLockedStack;
	friend class MutexLockedQueue;

	volatile atomic_word _next;

public:
	void* data[3];
	
	AtomicNode *Next() const
	{
		return (AtomicNode *) _next;
	}
	AtomicNode *Link(AtomicNode *next);
};
