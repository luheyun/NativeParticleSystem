#ifndef __EVENT_H
#define __EVENT_H

// Event synchronization object.

#if SUPPORT_THREADS

#if UNITY_WIN || UNITY_XENON
#	include "Winapi/PlatformEvent.h"
#elif HAS_EVENT_OBJECT
#	include "PlatformEvent.h"
#else
	//@TODO: Event implementation missing. Using a semaphore.
#	include "Semaphore.h"
	typedef Semaphore Event;
#endif

#endif // SUPPORT_THREADS

#endif // __EVENT_H
