#pragma once
// ------------------------------------------------------
// This helps with efficiency when kicking off jobs.
// It lets you dynamically build a batch of jobs and
// kick it in batches rather than kicking every job.
//
// This improves performance because:
// * Semaphore.Signal can be expensive on many platforms.
//	 Kicking in batch will never signal more than workerThreadCount times.
//   Some platforms have native support for Signal (int n) which can further reduce semaphore overhead.
// * When you have many e.g. 8 cores, it is not uncommon that the work to extract data on the
//   main thread takes as long or longer than the time to execute the jobs. This is not a bad thing per se, this situation can still yield great performance speedups.
//   But it is important to schedule all jobs in batch otherwise we will wake one thread up, execute the job, then fall asleep and shortly after wake up again etc.
//   This is commonly referred to as job starvation.
//   In this situation it is much better to wait until all job data in one batch has been fully extracted and only then kick off all jobs.
//   So there is good chunk of work for all the threads to actaully process.

#include "Jobs.h"

// This tells the JobBatchDispatcher that you will kick batches
// manually and it should not attempt to kick automatically for you
// as you build your job list.
#define kManualJobKick             -1

// Kick jobs in job thread count groups.
#define kBatchKickByJobThreadCount -2

class JobBatchDispatcher
{
public:
	JobBatchDispatcher(JobPriority priority = kNormalJobPriority, int jobsPerBatch = kManualJobKick );
	~JobBatchDispatcher();

	// Schedule independent jobs in batches to reduce overhead.
	// otherwise the API is the same as Jobs.h

	template<typename T>
	void ScheduleJobDepends(JobFence& fence, void jobFunc(T*), T* jobData, const JobFence& dependsOn)
		{ ScheduleJobDependsInternal(fence, reinterpret_cast<JobFunc*>(jobFunc), jobData, dependsOn); }
	
	template<typename T>
	void ScheduleJob(JobFence& fence, void jobFunc(T*), T* jobData)
		{ ScheduleJobDepends(fence, jobFunc, jobData, JobFence()); }

	template<typename T>
	void ScheduleJobForEach(JobFence& fence, void jobFunc(T*, unsigned), T* jobData, int iterationCount)
		{ ScheduleJobForEachInternal(fence, reinterpret_cast<JobForEachFunc*>(jobFunc), jobData, iterationCount, NULL, JobFence()); }
	template<typename T>
	void ScheduleJobForEachDepends(JobFence& fence, void jobFunc(T*, unsigned), T* jobData, int iterationCount, const JobFence& dependsOn)
		{ ScheduleJobForEachInternal(fence, reinterpret_cast<JobForEachFunc*>(jobFunc), jobData, iterationCount, NULL, dependsOn); }

	template<typename T>
	void ScheduleJobForEach(JobFence& fence, void jobFunc(T*, unsigned), T* jobData, int iterationCount, void combineFunc(T*))
		{ ScheduleJobForEachInternal(fence, reinterpret_cast<JobForEachFunc*>(jobFunc), jobData, iterationCount, reinterpret_cast<JobFunc*>(combineFunc), JobFence()); }
	template<typename T>
	void ScheduleJobForEachDepends(JobFence& fence, void jobFunc(T*, unsigned), T* jobData, int iterationCount, const JobFence& dependsOn, void combineFunc(T*))
		{ ScheduleJobForEachInternal(fence, reinterpret_cast<JobForEachFunc*>(jobFunc), jobData, iterationCount, reinterpret_cast<JobFunc*>(combineFunc), dependsOn); }
	void KickJobs();

private:
	inline JobGroupID GetJobQueueDependency(const JobFence& dependency);
	void ScheduleJobDependsInternal(JobFence& fence, JobFunc jobFunc, void* userData, const JobFence& dependsOn);
	void ScheduleJobForEachInternal(JobFence& fence, JobForEachFunc jobFunc, void* userData, int iterationCount, JobFunc combineFunc, const JobFence& dependsOn);
	void HandleJobKickInternal(class JobQueue& queue, JobFence& fence, JobGroup* group, int jobCount);

	JobGroup*		m_Head;
	JobGroup*		m_Tail;
	JobGroupID		m_Depends;

	int             m_JobsPerBatch;
	int             m_JobCount;
};
