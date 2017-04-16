#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#if SUPPORT_THREADS

#if UNITY_WIN
#	include "Winapi/PlatformSemaphore.h"
#elif UNITY_LINUX || UNITY_ANDROID || UNITY_PS3 || UNITY_BB10 || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL
#	include "Posix/PlatformSemaphore.h"
#else
#	include "PlatformSemaphore.h"
#endif

#include "Utilities/NonCopyable.h"

class Semaphore : public NonCopyable
{
public:
	Semaphore() { m_Semaphore.Create(); }
	~Semaphore() { m_Semaphore.Destroy(); }
	void Reset() { m_Semaphore.Reset(); }
	void WaitForSignal() { m_Semaphore.WaitForSignal(); }
	void Signal() { m_Semaphore.Signal(); }

	const PlatformSemaphore& GetPlatformSemaphore() const { return m_Semaphore; }

private:
	PlatformSemaphore m_Semaphore;
};

class SuspendableSemaphore : public NonCopyable
{
public:
	explicit SuspendableSemaphore(bool suspended = true) : m_SuspendedIndefinitely(false), m_Suspended(suspended) { }
	bool IsSuspended() const { return m_Suspended; }
	void Reset() { m_Semaphore.Reset(); }
	void WaitForSignal() { if (!m_Suspended) m_Semaphore.WaitForSignal(); };
	void Signal() { if (!m_Suspended) m_Semaphore.Signal(); }
	void Resume(bool reset = true);
	void Suspend(bool indefinitely = false);

private:
	volatile bool m_SuspendedIndefinitely;
	volatile bool m_Suspended;
	Semaphore m_Semaphore;
};

inline void SuspendableSemaphore::Resume(bool reset)
{
	if (reset)
		Reset();

	if (!m_SuspendedIndefinitely)
		m_Suspended = false;
}

inline void SuspendableSemaphore::Suspend(bool indefinitely)
{
	m_Suspended = true;
	if (indefinitely)
		m_SuspendedIndefinitely = indefinitely;

	m_Semaphore.Signal(); // release any waiting thread
}

#endif // SUPPORT_THREADS

#endif // __SEMAPHORE_H
