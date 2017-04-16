#pragma once

#include "Runtime/Threads/ExtendedAtomicOps.h"
#include "Runtime/Threads/Semaphore.h"
#include "Runtime/Utilities/NonCopyable.h"
#include "Runtime/Utilities/StaticAssert.h"

#if SUPPORT_THREADS

// Readers-writers lock class.
// It is a self-balancing readers-preferred lock which doesn't let writers starve a lot.
// Once it has writer in a queue it registers incoming readers as waiting readers. And, once active readers are gone,
// it starts processing a single writer. After that writer it flips waiting readers into active readers and processes them.
// The order in which readers and writers are resumed is not defined.
// Based on awesome Jeff Preshing's implementation at https://github.com/jpark37/cpp11-on-multicore/blob/master/common/rwlock.h
class ReadWriteLock : public NonCopyable
{
	// Scoped read lock template
	template<class TLock>
	class AutoReadLockHelper : public NonCopyable
	{
	public:
		AutoReadLockHelper(TLock& rwlock) : m_RWLock(rwlock) { rwlock.ReadLock(); }
		~AutoReadLockHelper() { m_RWLock.ReadUnlock(); }
	private:
		TLock& m_RWLock;
	};

	// Scoped write lock template
	template<class TLock>
	class AutoWriteLockHelper : public NonCopyable
	{
	public:
		AutoWriteLockHelper(TLock& rwlock) : m_RWLock(rwlock) { rwlock.WriteLock(); }
		~AutoWriteLockHelper() { m_RWLock.WriteUnlock(); }
	private:
		TLock& m_RWLock;
	};
public:
	typedef AutoReadLockHelper<ReadWriteLock> AutoReadLock;
	typedef AutoWriteLockHelper<ReadWriteLock> AutoWriteLock;

	ReadWriteLock() : m_ReadersWritesStatus(0) {}

	void ReadLock()
	{
		Status status, oldStatus;
		// memory_order_relaxed (and others) is conflicting with std::memory_order_relaxed because of "using namespace std" acrros the codebase
		// TODO: Remove global namespace specifier once "using namespace std" is removed
		oldStatus.data = atomic_load_explicit(&m_ReadersWritesStatus, ::memory_order_relaxed);
		do
		{
			status = oldStatus;
			// If we have active writers, put readers on a waiting list, so writers won't starve.
			if (status.writers > 0)
			{
				AssertMsg(status.waitingReaders < kMaxReadersCount, "ReadWriteLock::waitingReaders overflow!");
				status.waitingReaders++;
			}
			else
			{
				AssertMsg(status.readers < kMaxReadersCount, "ReadWriteLock::readers overflow!");
				status.readers++;
			}
		} while (!atomic_compare_exchange_weak_explicit(&m_ReadersWritesStatus, &oldStatus.data, status.data, ::memory_order_acquire, ::memory_order_relaxed));

		// If there were any active writers, wait for a signal to continue.
		if (oldStatus.writers > 0)
			m_ReadSemaphore.WaitForSignal();
	}

	void ReadUnlock()
	{
		Status status, oldStatus;
		oldStatus.data = atomic_load_explicit(&m_ReadersWritesStatus, ::memory_order_relaxed);
		do
		{
			status = oldStatus;
			AssertMsg(status.readers > 0, "ReadWriteLock::readers underflow!");
			status.readers--;
		} while (!atomic_compare_exchange_weak_explicit(&m_ReadersWritesStatus, &oldStatus.data, status.data, ::memory_order_acquire, ::memory_order_relaxed));
		if (oldStatus.readers == 1 && oldStatus.writers > 0)
			m_WriteSemaphore.Signal();
	}

	void WriteLock()
	{
		Status status, oldStatus;
		oldStatus.data = atomic_load_explicit(&m_ReadersWritesStatus, ::memory_order_relaxed);
		do
		{
			status = oldStatus;
			AssertMsg(status.writers < kMaxWritersCount, "ReadWriteLock::writers overflow!");
			status.writers++;
		} while (!atomic_compare_exchange_weak_explicit(&m_ReadersWritesStatus, &oldStatus.data, status.data, ::memory_order_acquire, ::memory_order_relaxed));
		if (oldStatus.readers > 0 || oldStatus.writers > 0)
			m_WriteSemaphore.WaitForSignal();
	}

	void WriteUnlock()
	{
		Status status, oldStatus;
		oldStatus.data = atomic_load_explicit(&m_ReadersWritesStatus, ::memory_order_relaxed);
		do
		{
			status = oldStatus;
			AssertMsg(oldStatus.writers > 0, "ReadWriteLock::writers underflow!");
			status.writers--;
			// If there are pending reads, we move waitingReaders to readers
			// and prevent new writers to acquire the lock.
			if (status.waitingReaders > 0)
			{
				status.readers = status.waitingReaders;
				status.waitingReaders = 0;
			}
		} while (!atomic_compare_exchange_weak_explicit(&m_ReadersWritesStatus, &oldStatus.data, status.data, ::memory_order_release, ::memory_order_relaxed));

		// In case we have awaiting readers, notify them first, so they won't starve.
		// Reader will notify a single writer once they are all done.
		if (status.readers > 0)
		{
			// Notify all readers
			int readers = status.readers;
			while (readers-- > 0)
				m_ReadSemaphore.Signal();
		}
		else if (status.writers > 0)
		{
			// Notify a single writer if we don't have any readers
			m_WriteSemaphore.Signal();
		}
	}

private:

	enum
	{
		kAtomicWordBits = sizeof(atomic_word) * 8,

		// As we have readers-preferred lock it's better to use more bits for readers counters.
		// And readers and waitingReaders should have the same amount of bits used.
		// So we give the most (11 bits - up to 2048 readers on x86) to them and the rest to writers.
		kReadersBits = (kAtomicWordBits - kAtomicWordBits / 3) / 2,
		kWritersBits = kAtomicWordBits - (kReadersBits * 2),

		kMaxReadersCount = (1 << kReadersBits) - 1,
		kMaxWritersCount = (1 << kWritersBits) - 1
	};
	CompileTimeAssert(kWritersBits + kReadersBits * 2 == kAtomicWordBits, "ReadWriteLock readers and writers counters should fit atomic_word");

	union Status
	{
		struct
		{
			atomic_word readers : kReadersBits;
			atomic_word waitingReaders : kReadersBits;
			atomic_word writers : kWritersBits;
		};
		atomic_word data;
	};

	volatile ALIGN_TYPE(8) atomic_word m_ReadersWritesStatus;
	Semaphore m_ReadSemaphore;
	Semaphore m_WriteSemaphore;
};

#else

// Dummy implementation for the platforms which don't support threads
class ReadWriteLock : public NonCopyable
{
	class AutoLockHelper : public NonCopyable
	{
	public:
		AutoLockHelper(ReadWriteLock&) {}
	};
public:

	typedef AutoLockHelper AutoReadLock;
	typedef AutoLockHelper AutoWriteLock;

	void ReadLock() {}
	void ReadUnlock() {}
	void WriteLock() {}
	void WriteUnlock() {}
};

#endif // SUPPORT_THREADS
