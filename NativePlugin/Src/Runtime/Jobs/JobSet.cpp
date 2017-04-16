#include "PluginPrefix.h"
#include "JobSet.h"
#include "Internal/JobQueue.h"

struct JobSetData
{
	int count;
	int usedCount;
	int didComplete;
	JobGroupID groups[1];
};

// During construction (BeginJobSet to EndJobSet we reuse fence.group.info as the pointer to the job set data)
// Thus may not call SyncFence between BeginJobSet and EndJobSet
void BeginJobSet (JobFence& fence, int jobCount)
{
#if ENABLE_JOB_SCHEDULER
	SyncFence(fence);

	if (jobCount == 0)
		return;

	size_t size = sizeof(JobSetData) + sizeof(JobGroupID) * jobCount;
	JobSetData* jobSetData = (JobSetData*)UNITY_MALLOC(kMemTempJobAlloc, size);
	jobSetData->count = jobCount;
	jobSetData->usedCount = 0;
	jobSetData->didComplete = 0;

	fence.group.info = reinterpret_cast<JobGroup*> (jobSetData);
#endif
}

#if ENABLE_JOB_SCHEDULER
static void WaitForJobSetJob(JobSetData* jobSetData)
{
	for (int i=0;i<jobSetData->usedCount;i++)
		GetJobQueue().WaitForJobGroup(jobSetData->groups[i]);

	UNITY_FREE(kMemTempJobAlloc, jobSetData);
}
#endif

void EndJobSet (JobFence& fence, JobPriority priority)
{
#if ENABLE_JOB_SCHEDULER
	JobSetData* jobSetData = reinterpret_cast<JobSetData*> (fence.group.info);

	if (jobSetData == NULL)
		return;

	fence.group.info = NULL;

	jobSetData->didComplete = 1;


	JobFence firstDependency;
	firstDependency = jobSetData->groups[0];
	ScheduleJobDepends(fence, WaitForJobSetJob, jobSetData, firstDependency, priority);

	ClearFenceWithoutSync(firstDependency);
#endif
}

static void AddToJobSet (JobFence& jobSet, JobFence& job)
{
	JobSetData* jobSetData = reinterpret_cast<JobSetData*> (jobSet.group.info);

	jobSetData->groups[jobSetData->usedCount] = job.group;
	jobSetData->usedCount++;
}

void ScheduleJobForEachJobSetInternal(JobFence& jobSet, JobForEachFunc concurrentJobFunc, void* userData, int iterationCount, const JobFence& dependsOn, JobFunc combineJobFunc, JobPriority priority)
{
#if ENABLE_JOB_SCHEDULER

	JobFence tempFence;
	ScheduleJobForEachDepends(tempFence, concurrentJobFunc, userData, iterationCount, dependsOn, combineJobFunc, priority);

	AddToJobSet(jobSet, tempFence);
	
	ClearFenceWithoutSync(tempFence);

#else
	
	ScheduleJobForEachDepends(jobSet, concurrentJobFunc, userData, iterationCount, dependsOn, combineJobFunc, priority);
#endif
}
