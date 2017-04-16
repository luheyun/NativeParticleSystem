#pragma once

#include "Runtime/Threads/AtomicQueue.h"
#include "Runtime/Threads/MutexLockedQueue.h"
#include "Runtime/Threads/MutexLockedStack.h"

#if defined(ATOMIC_HAS_QUEUE)

	typedef AtomicNode				ConcurrentNode;
	typedef AtomicStack				ConcurrentStack;
	typedef AtomicQueue				ConcurrentQueue;

	FORCE_INLINE ConcurrentStack* CreateConcurrentStack()
	{
		return CreateAtomicStack();
	}

	FORCE_INLINE void DestroyConcurrentStack(ConcurrentStack* ptr)
	{
		DestroyAtomicStack(ptr);
	}
	
	FORCE_INLINE ConcurrentQueue* CreateConcurrentQueue()
	{
		return CreateAtomicQueue();
	}

	FORCE_INLINE void DestroyConcurrentQueue(ConcurrentQueue* ptr)
	{
		DestroyAtomicQueue(ptr);
	}

#else

	typedef AtomicNode				ConcurrentNode;
	typedef MutexLockedStack		ConcurrentStack;
	typedef MutexLockedQueue		ConcurrentQueue;

	FORCE_INLINE ConcurrentStack* CreateConcurrentStack()
	{
		return CreateMutexLockedStack();
	}

	FORCE_INLINE void DestroyConcurrentStack(ConcurrentStack* ptr)
	{
		DestroyMutexLockedStack(ptr);
	}
	
	FORCE_INLINE ConcurrentQueue* CreateConcurrentQueue()
	{
		return CreateMutexLockedQueue();
	}

	FORCE_INLINE void DestroyConcurrentQueue(ConcurrentQueue* ptr)
	{
		DestroyMutexLockedQueue(ptr);
	}

#endif