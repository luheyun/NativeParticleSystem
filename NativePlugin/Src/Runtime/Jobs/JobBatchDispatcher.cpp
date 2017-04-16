#include "PluginPrefix.h"
#include "JobBatchDispatcher.h"
#include "Internal/JobQueue.h"

#define ENABLE_JOB_BATCH ENABLE_JOB_SCHEDULER

JobBatchDispatcher::JobBatchDispatcher(JobPriority priority, int jobsPerBatch)
	: m_Head(NULL)
	, m_Tail(NULL)
	, m_JobsPerBatch(jobsPerBatch)
	, m_JobCount(0)
{
	JobQueue& queue = GetJobQueue();

	//Assert((priority & kHighJobPriority) == 0);
	if ((priority & kGuaranteeNoSyncFence) == 0)
		m_Depends = queue.GetAnyJobGroupID ();

	if( m_JobsPerBatch == kBatchKickByJobThreadCount )
	{
		m_JobsPerBatch  = std::max<int>(queue.GetThreadCount (), 1);
	}
}

inline JobGroupID JobBatchDispatcher::GetJobQueueDependency(const JobFence& dependency)
{
	if (dependency.group.info != NULL)
		return dependency.group;
	else
		return m_Depends;
}

JobBatchDispatcher::~JobBatchDispatcher()
{
	if( m_Head && m_JobCount > 0 )
		KickJobs();
}

#if ENABLE_JOB_BATCH
void JobBatchDispatcher::ScheduleJobDependsInternal(JobFence& fence, JobFunc jobFunc, void* userData, const JobFence& dependsOn)
{
	SyncFence(fence);
	JobQueue& queue = GetJobQueue ();

	JobGroup* group = queue.CreateJobBatch(jobFunc, userData, GetJobQueueDependency (dependsOn), m_Tail);
	HandleJobKickInternal (queue, fence, group, 1);
}

void JobBatchDispatcher::ScheduleJobForEachInternal(JobFence& fence, JobForEachFunc jobFunc, void* userData, int iterationCount, JobFunc combineFunc, const JobFence& dependsOn)
{
	SyncFence(fence);
	JobQueue& queue = GetJobQueue ();

	JobGroup* group = queue.CreateForEachJobBatch(jobFunc, userData, iterationCount, combineFunc, GetJobQueueDependency (dependsOn), m_Tail);
	HandleJobKickInternal (queue, fence, group, iterationCount);
}

void JobBatchDispatcher::HandleJobKickInternal(JobQueue& queue, JobFence& fence, JobGroup* group, int jobCount)
{
	if( m_Head == NULL )
		m_Head = group;
	m_Tail = group;

	// We can create a group ID for each job externally like so. These are individual
	// fences for each job.

	fence.group = queue.GetJobGroupID(group);

	m_JobCount += jobCount;
	if( m_JobsPerBatch != kManualJobKick && m_JobCount >= m_JobsPerBatch )
	{
		KickJobs();
	}
}

void JobBatchDispatcher::KickJobs()
{
	if( NULL != m_Head && m_JobCount > 0 )
	{
		JobQueue& queue = GetJobQueue();
		queue.ScheduleGroups( (JobGroup*)m_Head, (JobGroup*)m_Tail );

		m_JobCount = 0;
		m_Head     = NULL;
		m_Tail     = NULL;
	}
}

#else // ENABLE_JOB_BATCH

void JobBatchDispatcher::ScheduleJobDependsInternal(JobFence& fence, JobFunc* jobFunc, void* userData, const JobFence& dependsOn)
{
	// $ dependsOn is not used because, with no scheduler, everything runs immediately (and so 'dependsOn' has already completed)

	::ScheduleJob(fence, jobFunc, userData);
}

void JobBatchDispatcher::ScheduleJobForEachInternal(JobFence& fence, JobForEachFunc jobFunc, void* userData, int iterationCount, JobFunc combineFunc, const JobFence& dependsOn)
{
	// $ dependsOn is not used because, with no scheduler, everything runs immediately (and so 'dependsOn' has already completed)
	::ScheduleJobForEach(fence, jobFunc, userData, iterationCount, combineFunc);
}

void JobBatchDispatcher::KickJobs()
{
}

#endif // ENABLE_JOB_BATCH
