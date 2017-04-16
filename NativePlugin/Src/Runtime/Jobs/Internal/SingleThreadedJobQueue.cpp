#include "PluginPrefix.h"
#include "SingleThreadedJobQueue.h"

#if !ENABLE_JOB_SCHEDULER

static JobQueue* gJobQueue = NULL;

EXPORT_COREMODULE JobQueue& GetJobQueue ()
{
	return *gJobQueue;
}

void CreateJobQueue (const char* queueName, const char* workerName)
{
	gJobQueue = UNITY_NEW(JobQueue, kMemThread)(0, -1);
}

void DestroyJobQueue ()
{
	UNITY_DELETE(gJobQueue, kMemThread);
}

bool JobQueueCreated ()
{
	return (gJobQueue != NULL);
}

class JobGroup
{
public:
	JobGroup( JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete)
		: m_Func(func), m_UserData(userData), m_Count(n), m_Complete(complete) {}

	JobForEachFunc* m_Func;
	void*           m_UserData;
	int             m_Count;
	JobFunc*        m_Complete;
};

JobGroup* JobQueue::CreateJobsForEach (JobForEachFunc* jobFunction, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobGroup* prev)
{
	return UNITY_NEW(JobGroup, kMemJobScheduler)( jobFunction, userData, n, complete );
}

JobGroupID JobQueue::ScheduleGroup (JobGroup* group, JobQueuePriority priority)
{
	for (int i = 0; i < group->m_Count; i++)
		group->m_Func(group->m_UserData, i);

	if (group->m_Complete)
		group->m_Complete(group->m_UserData);

	UNITY_DELETE(group, kMemJobScheduler);

	return JobGroupID();
}

JobGroupID ScheduleMainThreadJob (JobFunc* func, void* userData, JobGroupID dependency)
{
	func (userData);
	return JobGroupID();
}

#endif
