#include "PluginPrefix.h"
#include "Jobs.h"
#include "Internal/JobQueue.h"

static inline void BeginFence (JobFence& fence)
{
	// Jobs are not always guaranteed to be consumed.
	// For example a character might be evaluated, but then turns out to be visible
	// and thus the job data will never be read

	SyncFence (fence);
}

void ClearFenceWithoutSync (JobFence& fence)
{
	fence.group.info = 0;
}

///@TODO: THis integrate job Func thing doens't work at all.
//        Either remove it or make it 3 states...

void CompleteFenceInternal (JobFence& fence)
{
	// unless fence is passed by value, there is no guaranty that it's not changed between the test and this function call
	//Assert(fence.group.info != NULL);
	GetJobQueue ().WaitForJobGroup (fence.group);
	fence.group.info = NULL;
}

void SyncFenceNoClear (const JobFence& fence)
{
	GetJobQueue ().WaitForJobGroup (fence.group);
}

bool IsFenceDone (const JobFence& fence)
{
	if (fence.group.info == NULL)
		return true;
	return GetJobQueue ().HasJobGroupCompleted(fence.group);
}

inline JobQueue::JobQueuePriority GetJobQueuePriority(JobPriority pri)
{
	return (JobQueue::JobQueuePriority)(pri & ~kGuaranteeNoSyncFence);
}

inline JobGroupID GetJobQueueDependency (JobQueue& queue, JobPriority pri)
{
	if (pri & kGuaranteeNoSyncFence)
		return JobGroupID();
	else
		return queue.GetAnyJobGroupID ();
}

inline JobGroupID GetJobQueueDependency (JobQueue& queue, const JobFence& dependency, JobPriority pri)
{
	if (dependency.group.info != NULL)
		return dependency.group;
	else
		return GetJobQueueDependency (queue, pri);
}

void ScheduleJobInternal(JobFence& fence, JobFunc* jobFunc, void* userData, JobPriority priority)
{
	BeginFence(fence);

	JobQueue& queue = GetJobQueue ();

	fence.group = queue.ScheduleJob(jobFunc, userData, GetJobQueueDependency(queue, priority), GetJobQueuePriority(priority));
}

void ScheduleJobDependsInternal(JobFence& fence, JobFunc* jobFunc, void* userData, const JobFence& dependsOn, JobPriority priority)
{
	#if ENABLE_JOB_SCHEDULER

	BeginFence(fence);
	
	JobQueue& queue = GetJobQueue ();
	
	fence.group = queue.ScheduleJob(jobFunc, userData, GetJobQueueDependency(queue, dependsOn, priority), GetJobQueuePriority(priority));
	#else
	ScheduleJob(fence, jobFunc, userData, priority);
	#endif
}

void ScheduleDifferentJobsConcurrentDepends (JobFence& fence, const Job* jobs, int jobCount, const JobFence& dependsOn, JobPriority priority )
{
	if (jobCount == 0)
		return;

	BeginFence(fence);

	#if ENABLE_JOB_SCHEDULER
	JobQueue& queue = GetJobQueue ();
	JobGroup *group = queue.CreateGroup(jobCount, GetJobQueueDependency(queue, dependsOn, priority));
	JobInfo *info = (JobInfo *) group->FirstJob();
	for(int i = 0; i < jobCount; i++)
	{
		info = info->Set(jobs[i].jobFunction, jobs[i].userData);
	}
	fence.group = queue.ScheduleGroup(group, GetJobQueuePriority(priority));
	#else
	for(int i = 0; i < jobCount; i++)
		jobs[i].jobFunction (jobs[i].userData);
	#endif
}

void ScheduleDifferentJobsConcurrent (JobFence& fence, const Job* jobs, int jobCount, JobPriority priority )
{
	JobFence noDependency;
	ScheduleDifferentJobsConcurrentDepends (fence, jobs, jobCount, noDependency, priority);
}

void ScheduleJobForEachInternal(JobFence& fence, JobForEachFunc* concurrentJobFunc, void* userData, int iterationCount, JobFunc* combineJobFunc, JobPriority priority)
{
	BeginFence(fence);

	JobQueue& queue = GetJobQueue ();
	
	fence.group = queue.ScheduleJobsForEach(concurrentJobFunc, userData, iterationCount, combineJobFunc, GetJobQueueDependency(queue, priority), GetJobQueuePriority(priority));
}

void ScheduleJobForEachDependsInternal(JobFence& fence, JobForEachFunc* concurrentJobFunc, void* userData, int iterationCount, const JobFence& dependsOn, JobFunc* combineJobFunc, JobPriority priority)
{
	BeginFence(fence);

	JobQueue& queue = GetJobQueue ();
	
	fence.group = queue.ScheduleJobsForEach(concurrentJobFunc, userData, iterationCount, combineJobFunc, GetJobQueueDependency(queue, dependsOn, priority), GetJobQueuePriority(priority));
}

bool ExecuteOneJobInMainThread()
{
#if ENABLE_JOB_SCHEDULER
	JobQueue& queue = GetJobQueue ();
	return queue.ExecuteOneJobOnMainThread();
#else
	return false;
#endif
}
