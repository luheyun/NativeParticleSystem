#ifndef __PLATFORMSEMAPHORE_H
#define __PLATFORMSEMAPHORE_H

#if SUPPORT_THREADS

#ifndef SEMAPHORE_API_PTHREAD
#define SEMAPHORE_API_PTHREAD (UNITY_LINUX || UNITY_ANDROID || UNITY_PS3 || UNITY_BB10 || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL)
#endif

#endif // SUPPORT_THREADS

#if SEMAPHORE_API_PTHREAD

#include <semaphore.h>
#include <errno.h>

#include "Runtime/Utilities/Word.h"
#include "Runtime/Utilities/NonCopyable.h"

class PlatformSemaphore : public NonCopyable
{
	friend class Semaphore;
protected:
	void Create();
	void Destroy();

	void WaitForSignal();
	void Signal();
	void Reset() { Destroy(); Create(); }

private:
	sem_t m_Semaphore;
};

#define REPORT_SEM_ERROR(action) ErrorStringMsg ("Failed to %s a semaphore (%s)\n", action, strerror (errno))

	inline void PlatformSemaphore::Create() { if (sem_init(&m_Semaphore, 0, 0) == -1) REPORT_SEM_ERROR ("open"); }
	inline void PlatformSemaphore::Destroy() { if (sem_destroy(&m_Semaphore) == -1) REPORT_SEM_ERROR ("destroy"); }
	inline void PlatformSemaphore::WaitForSignal() {
		int ret = 0;
		while ((ret = sem_wait(&m_Semaphore)) == -1 && errno == EINTR)
			continue;

		if( ret == -1 )
			REPORT_SEM_ERROR ("wait on");
	}
	inline void PlatformSemaphore::Signal() {
		if (sem_post(&m_Semaphore) == -1)
			REPORT_SEM_ERROR ("post to");
	}

#undef REPORT_SEM_ERROR

#endif // SEMAPHORE_API_PTHREAD
#endif // __PLATFORMSEMAPHORE_H
