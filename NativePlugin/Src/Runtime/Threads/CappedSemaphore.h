#ifndef __UNITY_CAPPED_SEMAPHORE_H
#define __UNITY_CAPPED_SEMAPHORE_H

#if SUPPORT_THREADS

#include "AtomicOps.h"
#include "Semaphore.h"
#include "ThreadUtility.h"
#include "Utilities/NonCopyable.h"

#include "limits.h"

/// Semaphore implementation that is capped to a max value.
/// Any call to Signal once the semaphore reached it's max value will be ignored.
///
/// This can be useful when the semaphore is used as a fence rather than a counter.

class CappedSemaphore : public NonCopyable
{
public:
	CappedSemaphore(size_t maxValue)
	: m_CurrentValue(0)
	, m_MaxValue(maxValue)
	{
	}

	void Signal(int count = 1)
	{
		UnityMemoryBarrier();

		int oldValue, newValue;
		do
		{
			oldValue = m_CurrentValue;
			newValue = oldValue + count;
			newValue = newValue > m_MaxValue ? m_MaxValue : newValue;

			if (newValue == oldValue)
				return;
		}
		while (!AtomicCompareExchange(&m_CurrentValue, newValue, oldValue));

		for (int i = oldValue; i < newValue; ++i)
		{
			if (i < 0)
				m_Semaphore.Signal();
		}
	}

	void WaitForSignal()
	{
		int oldValue, newValue;
		do
		{
			oldValue = m_CurrentValue;
			newValue = oldValue - 1;
			if (newValue == INT_MIN)
			{
				return;
			}
		}
		while (!AtomicCompareExchange(&m_CurrentValue, newValue, oldValue));

		if (newValue < 0)
			m_Semaphore.WaitForSignal();

		UnityMemoryBarrier();
	}

private:
	volatile int    m_CurrentValue;
	const size_t    m_MaxValue;
	Semaphore       m_Semaphore;
};

#endif // SUPPORT_THREADS

#endif // __UNITY_CAPPED_SEMAPHORE_H
