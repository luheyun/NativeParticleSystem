#include "PluginPrefix.h"
#include "Mutex.h"

#if SUPPORT_WORKER_THREAD_MUTEX_LOCK_ASSERT

#include "ThreadSpecificValue.h"

// @todo: Remove g_DidInitializeAllowMutexLock when we no longer allocate memory during global constructor initialization.
// This should be fixed. But until it is we need to prevent gAllowMutexLockRefCount until global construction has completed.
static int                      g_DidInitializeAllowMutexLock = 0;
static UNITY_TLS_VALUE(int)		g_AllowMutexLockRefCount;

Mutex::AutoAllowWorkerThreadLock::AutoAllowWorkerThreadLock(bool allow)
	: m_OldRefCount(g_AllowMutexLockRefCount)
{
	g_DidInitializeAllowMutexLock = 1;

	bool collision = false;
	if (allow)
	{
		collision = g_AllowMutexLockRefCount < 0;
		++g_AllowMutexLockRefCount;
	}
	else
	{
		collision = g_AllowMutexLockRefCount > 0;
		--g_AllowMutexLockRefCount;
	}

	//AssertMsg(!collision, "Two functions on this thread's stack are fighting over whether mutex locks should be allowed or not");

	m_NewRefCount = g_AllowMutexLockRefCount;
}

Mutex::AutoAllowWorkerThreadLock::~AutoAllowWorkerThreadLock()
{
	//AssertMsg(g_AllowMutexLockRefCount == m_NewRefCount, "Invalid interleaved acquire/release of allow-lock refcount");

	g_AllowMutexLockRefCount = m_OldRefCount;
}

#endif // SUPPORT_WORKER_THREAD_MUTEX_LOCK_ASSERT

// $ do not "optimize" these away by deleting them or Xbone, 360, and PS3 CgBatch\...\Interface.cpp will fail to build
// (TODO: fix them to be like the other platforms)
Mutex::Mutex() {}
Mutex::~Mutex() {}

void Mutex::Lock()
{
#	if SUPPORT_WORKER_THREAD_MUTEX_LOCK_ASSERT
	if (g_DidInitializeAllowMutexLock && g_AllowMutexLockRefCount < 0)
	{
		// If you get an assert here, see docs for AutoAllowWorkerThreadLock in Mutex.h
		//AssertBreak(false);
	}
#	endif

#	if SUPPORT_THREADS
	m_Mutex.Lock();
#	endif
}

void Mutex::Unlock()
{
#	if SUPPORT_THREADS
	m_Mutex.Unlock();
#	endif
}

bool Mutex::TryLock()
{
	bool locked = true;

#	if SUPPORT_THREADS
	locked = m_Mutex.TryLock();
#	endif

	return locked;
}

bool Mutex::IsLocked()
{
#	if SUPPORT_THREADS
	if (!m_Mutex.TryLock())
		return true;

	m_Mutex.Unlock();
#	endif

	return false;
}

void Mutex::BlockUntilUnlocked()
{
	Lock();
	Unlock();
}
