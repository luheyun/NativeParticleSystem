#pragma once

#if SUPPORT_THREADS

#include "Runtime/Utilities/NonCopyable.h"
#include <pthread.h>

class Monitor : public NonCopyable
{
public:
	class Lock : public NonCopyable
	{
	public:
		Lock(Monitor& monitor)
		: m_Monitor(monitor)
		{
			pthread_mutex_lock(&m_Monitor.mutex);
		}

		~Lock()
		{
			pthread_mutex_unlock(&m_Monitor.mutex);
		}

		void Wait()
		{
			pthread_cond_wait(&m_Monitor.cond, &m_Monitor.mutex);
		}
	private:
		Monitor& m_Monitor;
	};

	Monitor()
	{
		pthread_mutexattr_t    attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&mutex, &attr);

		pthread_cond_init(&cond, NULL);
	}

	~Monitor()
	{
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&cond);
	}

	void Signal()
	{
		pthread_cond_signal(&cond);
	}

	void Broadcast()
	{
		pthread_cond_broadcast(&cond);
	}

	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
};

#endif // SUPPORT_THREADS
