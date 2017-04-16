#ifndef SINGLE_THREADED_JOB_QUEUE_H
#define SINGLE_THREADED_JOB_QUEUE_H

#include "JobQueue.h"

#if !ENABLE_JOB_SCHEDULER

struct JobInfo;
class JobGroup;

class EXPORT_COREMODULE JobQueue
{
public:

	enum JobQueueFlags
	{
		kUseProfiler = 1 << 0,
		kAllowMutexLocks = 1 << 1,
		kUseProfilerAndAllowMutexLocks = kUseProfiler | kAllowMutexLocks
	};
	enum ShutdownMode
	{
		kShutdownImmediate = 1,
		kShutdownWaitForAllJobs
	};

	JobQueue (int numthreads, size_t tempAllocSize, int startProcessor = -1, JobQueueFlags flags = kUseProfiler,  const char* queueName = "Job Queue", const char* workerName = "Worker Thread") { }
	~JobQueue () {}
	
	enum JobQueuePriority
	{
		kNormalJobPriority = 0,
		kHighJobPriority = 1,
	};

	JobGroupID ScheduleJob (JobFunc* func, void* userData, JobGroupID dependency, JobQueuePriority priority)
	{
		func (userData);
		return JobGroupID();
	}

	JobGroupID ScheduleJobsForEach (JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobQueuePriority priority)
	{
		for (int i=0;i<n;i++)
			func(userData, i);

		if (complete)
			complete(userData);

		return JobGroupID();

	}

	JobGroup* CreateJobsForEach (JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobGroup* prev = NULL);
	JobGroupID ScheduleGroup (JobGroup* group, JobQueuePriority priority);

	JobGroupID GetAnyJobGroupID () const	{ return JobGroupID (); }

	void WaitForJobGroup (JobGroupID group) { }

	bool HasJobGroupCompleted (JobGroupID group) { return true; }

	unsigned GetThreadCount () const { return 0; }

	#if ENABLE_JOB_SCHEDULER_PROFILER
	void EndProfilerFrame (UInt32 frameID) {}
	#endif

	void Wake (unsigned n) { }

	void SetThreadPriority (int priority) { }

	bool ExecuteOneJob() { return false; }

	bool ExecuteJobFromHighPriorityStack() { return false; }

	void Shutdown(ShutdownMode shutdownMode) { }
};

#if !UNITY_EXTERNAL_TOOL
void CreateJobQueue (const char* queueName, const char* workerName);
void DestroyJobQueue ();
bool JobQueueCreated ();
EXPORT_COREMODULE JobQueue& GetJobQueue ();
JobGroupID ScheduleMainThreadJob (JobFunc* func, void* userData, JobGroupID dependency);
#endif

#endif

#endif // SINGLE_THREADED_JOB_QUEUE_H
