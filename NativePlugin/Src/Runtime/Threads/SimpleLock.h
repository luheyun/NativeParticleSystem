#ifndef SIMPLE_LOCK_H
#define SIMPLE_LOCK_H

#if SUPPORT_THREADS

#include "ExtendedAtomicOps.h"
#include "Semaphore.h"

// Simple, non-recursive mutual exclusion lock. Efficient when there is low contention.
// First tries to attain lock using atomic ops, then waits for lock using semaphores.
// Same idea as described here: http://preshing.com/20120226/roll-your-own-lightweight-mutex

class SimpleLock : public NonCopyable
{
public:
	SimpleLock() : m_Count(0) {}

	class AutoLock : public NonCopyable
	{
	public:
		AutoLock( SimpleLock& lock ) : m_Lock(lock)
		{
			m_Lock.Lock();
		}

		~AutoLock()
		{
			m_Lock.Unlock();
		}

	private:
		SimpleLock& m_Lock;
	};

	void Lock()
	{
		if (atomic_fetch_add_explicit(&m_Count, 1, memory_order_acquire) > 0)
			m_Semaphore.WaitForSignal();
	}

	void Unlock()
	{
		if (atomic_fetch_sub_explicit(&m_Count, 1, memory_order_release) > 1)
			m_Semaphore.Signal();
	}

private:
	int 					m_Count;
	Semaphore 				m_Semaphore;
};

#endif // SUPPORT_THREADS
#endif
