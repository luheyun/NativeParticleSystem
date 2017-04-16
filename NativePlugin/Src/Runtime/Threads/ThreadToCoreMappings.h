#pragma once

// Map of all known thread priorities and assignments for this platform.
// Please place all defines in here to ensure we have a single place to
// go when testing various thread configurations from platform to platform.

// Ideally we would actually expose this in a config file
// letting AAA studios that really know what they are doing experiment
// and tweak their thread profile for the type of game they are making.

#if UNITY_PLATFORM_THREAD_TO_CORE_MAPPING || UNITY_XENON || UNITY_OSX
	#include "Threads/ThreadToCoreMappings.h"
#else
	// Graphics Worker Thread
	#define GFX_DEVICE_WORKER_PROCESSOR     DEFAULT_UNITY_THREAD_PROCESSOR
#if UNITY_WINRT
	#define GFX_DEVICE_WORKER_PRIORITY      (kHighPriority)
#else
	#define GFX_DEVICE_WORKER_PRIORITY      (kNormalPriority)
#endif

	// Job Scheduler, this is a range and indicates the starting core
	#define JOB_SCHEDULER_START_PROCESSOR   DEFAULT_UNITY_THREAD_PROCESSOR
	// Maximum number of threads allowed by the job scheduler.
	// Use one less than processor count because there is at least one other thread active, such as the main or render thread.
	// Overallocating is safe because we always idle to free up the processor when waiting. This prevents priority inversion.
	// It also makes it safe to use the hyperthreaded processor count instead of only physical cores.
	#define JOB_SCHEDULER_MAX_THREADS       (systeminfo::GetProcessorCount() - 1)

	#define ENLIGHTEN_WORKER_MAX_THREADS    (systeminfo::GetProcessorCount())   // Maximum number of threads allowed for Enlighten
	// Loading Thread, ideally this goes on a core we will be using less during loading.
	// Job thread cores are the best candidates for that.
	#define LOADING_THREAD_WORKER_PROCESSOR 1
	#define LOADING_THREAD_WORKER_NORMAL_PRIORITY   (kBelowNormalPriority)
	#define LOADING_THREAD_WORKER_HIGH_PRIORITY     (kHighPriority)

	// Audio threads. Currently matches the FMOD allowance and only actually used
	// when overriden.
	#define AUDIO_THREAD_FEEDER             DEFAULT_UNITY_THREAD_PROCESSOR
	#define AUDIO_THREAD_MIXER              DEFAULT_UNITY_THREAD_PROCESSOR
	#define AUDIO_THREAD_NONBLOCKING        DEFAULT_UNITY_THREAD_PROCESSOR
	#define AUDIO_THREAD_STREAM             DEFAULT_UNITY_THREAD_PROCESSOR
	#define AUDIO_THREAD_FILE               DEFAULT_UNITY_THREAD_PROCESSOR
	#define AUDIO_THREAD_GEOMETRY           DEFAULT_UNITY_THREAD_PROCESSOR

	#define MONO_WORKER_THREAD_PROCESSOR    DEFAULT_UNITY_THREAD_PROCESSOR
#endif
