#pragma once

#include "ExtendedAtomicOps.h"
#include "Thread.h"

// A simple non-recursive read/write spin lock
// * Multiple readers are allowed at the same time.
// * only a single writer is allowed at once time and while there is a writer, no reader is allowed
// * No semaphore is used, instead we do spinning, thus it is an extremely bad idea when the write lock is taken for more than a couple of cycles.

//   Generally speaking use this only if:
//   * WriteLock is a rare thing to be called
//   * All locks are generally very short

class ReadWriteSpinLock
{
#define WRITE_LOCK_STATE 0xFFFFFFF1

public:

	class AutoReadLock : NonCopyable
	{
		ReadWriteSpinLock& m_SpinLock;

	public:
		AutoReadLock(ReadWriteSpinLock& spinLock) : m_SpinLock(spinLock) { m_SpinLock.ReadLock(); }
		~AutoReadLock() { m_SpinLock.ReadUnlock(); }
	};


	class AutoWriteLock : NonCopyable
	{
		ReadWriteSpinLock& m_SpinLock;

	public:
		AutoWriteLock(ReadWriteSpinLock& spinLock) : m_SpinLock(spinLock) { m_SpinLock.WriteLock(); }
		~AutoWriteLock() { m_SpinLock.WriteUnlock(); }
	};

	ReadWriteSpinLock()
	{
		m_Counter = 0;
	}

	void ReadLock()
	{
		// We assume the most common case is completely uncontended locks (no read / write)
		// Thus we early out with hopefully a single atomic-op (Assuming that m_Counter is in fact 0)
		// (if it is not then we hope to hit a read lock, which means one more atomic_compare_exchange_weak_explicit call)
		// in the worst case, we will do spinning while the write lock is taken 
		atomic_word oldVal = 0;
		while (!atomic_compare_exchange_weak_explicit (&m_Counter, &oldVal, oldVal+1, ::memory_order_acquire, ::memory_order_relaxed))
		{
			// Write lock... relax while waiting for write lock to be returned
			// When it gets return it will be set back to 0, so thats what we try to CAS as the old value next iteration
			if (oldVal == WRITE_LOCK_STATE)
			{
				oldVal = 0;
				atomic_pause();	
			}
			else
			{
				// Else we just use the old value and use CAS to increment it above in the while() condition
			}
		}
	}

	void ReadUnlock()
	{
		DebugAssert(atomic_load_explicit(&m_Counter, ::memory_order_acquire) != WRITE_LOCK_STATE);
		atomic_fetch_sub_explicit(&m_Counter, 1, ::memory_order_release);
	}

	void WriteLock()
	{
		atomic_word oldVal = 0;
		while (!atomic_compare_exchange_weak_explicit (&m_Counter, &oldVal, WRITE_LOCK_STATE, ::memory_order_acquire, ::memory_order_relaxed))
		{
			oldVal = 0;
			atomic_pause();
		}
	}

	void WriteUnlock()
	{
		DebugAssert(atomic_load_explicit(&m_Counter, ::memory_order_acquire) == WRITE_LOCK_STATE);
		atomic_store_explicit(&m_Counter, 0, ::memory_order_release);
	}

private:

	atomic_word m_Counter;
};