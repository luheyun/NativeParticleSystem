#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "JobGroupID.h"
#include "Threads/AtomicQueue.h"
#include "Threads/Semaphore.h"
#include "Threads/CappedSemaphore.h"
#include "Threads/Thread.h"
#include "Utilities/EnumFlags.h"
#include "Utilities/NonCopyable.h"

typedef void JobFunc(void* userData);
typedef void JobForEachFunc(void* userData, unsigned index);

// On single-CPU platforms, this should never be used
//#if (ENABLE_MULTITHREADED_CODE || ENABLE_MULTITHREADED_SKINNING) && defined(ATOMIC_HAS_QUEUE)
//#define ENABLE_JOB_SCHEDULER 1
//#else
#define ENABLE_JOB_SCHEDULER 1
//#endif

#if ENABLE_JOB_SCHEDULER

struct JobInfo
{
	friend class JobGroup;
	friend class JobQueue;

private:

	JobInfo*	_next;
	JobFunc*	jobFunction;
	void*		userData;
	UInt32		forEachIndex : 31;
	UInt32		isForEach : 1;
	JobGroup*	group;
	
	inline void Link (JobInfo* next)
	{
		_next = next;
	}

public:
	inline JobInfo* Set (JobFunc* function, void* userData)
	{
		this->jobFunction = function;
		this->userData = userData;
		this->isForEach = 0;
		return _next;
	}
	inline JobInfo* Next ()
	{
		return _next;
	}
};

class EXPORT_COREMODULE JobQueue : NonCopyable
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
	JobQueue (unsigned numthreads, size_t tempAllocSize, int startProcessor, JobQueueFlags flags,  const char* queueName, const char* workerName);
	~JobQueue ();

	// keep this matching Jobs.h:JobPriority
	enum JobQueuePriority
	{
		kNormalJobPriority = 0,
		kHighJobPriority = 1 << 0,
		kMainJobPriority = 1 << 2,
	};

	/// IMPORTANT DETAILS
	/// * Passing the right JobGroupID dependency value (THIS IS MORE INTRICATE THAN YOU WOULD THINK)
	///   - If a job has no explicit dependency and no SyncFence or WaitForGroup calls inside the job, then empty JobGroupID can be passed.
	///     You have to be absolutely 100% sure.
	///   - If a job might call SyncFence, then GetAnyJobGroupID() has to be passed.
	///   - If a job has an explicit dependency, then you pass that dependency
	///   Failure to comply will result in a deadlock. You have been warned.

	/// Why?
	/// While we are waiting for jobs in a group to complete, we consume other jobs.
	/// This is only safe if there are no dependencies on that job.
	/// So basically this is to avoid recursively trying to execute a group for which the current group is a dependency.

	JobGroup* CreateGroup (unsigned jobs, JobGroupID dependency);
	JobGroupID ScheduleGroup (JobGroup* group, JobQueuePriority priority);

	JobGroupID ScheduleJob (JobFunc* func, void* userData, JobGroupID dependency, JobQueuePriority priority);
	JobGroupID ScheduleJobsForEach (JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobQueuePriority priority);

	// Create jobs for batching and scheduling later.
	// Only supports normal priority
	// (Supporting high priority jobs in batch is possible but complicated work)
	JobGroup* CreateJobBatch (JobFunc* func, void* userData, JobGroupID dependency, JobGroup* prev);
	JobGroup* CreateForEachJobBatch (JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobGroup* prev);
	void ScheduleGroups (JobGroup* first, JobGroup* last);

	JobGroupID GetJobGroupID (JobGroup* group);
	
	void Cleanup();
	void WaitForJobGroup (JobGroupID group, bool execMain = false);
	int SignalOnFinish(JobGroupID gid, Semaphore *sema, bool always = true);

	bool HasJobGroupCompleted (JobGroupID group);

	unsigned GetThreadCount () const { return m_ThreadCount; }

	// dummy JobGroupID for unknown dependencies
	JobGroupID GetAnyJobGroupID () const	{ return m_AnyJobGroupID; }

	void Wake (unsigned n);

	// Set the thread prioirity using Thread::ThreadPriority
	void SetThreadPriority (int priority);
	
	bool ExecuteOneJob ();

	void Shutdown(ShutdownMode shutdownMode);

	bool ExecuteOneJobOnMainThread();

private:

	friend class JobGroup;
	
	friend void CreateJobQueue (const char* queueName, const char* workerName);
	friend void DestroyJobQueue ();

	inline void ExecuteJobFunc (JobInfo* info);

	JobGroup* CreateJobsForEach (JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency);

	JobGroup* CreateJob  (JobFunc* func, void* userData, JobGroupID dependency);

	int ExecuteJobFromMainQueue ();
	bool ExecuteJobFromHighPriorityStack ();
	bool ExecuteJobFromQueue ();
	
	void ScheduleDependencies (JobGroup* group, JobInfo *first, JobInfo *last);

	int Exec (JobInfo* info, atomic_word v, int count);

	void ResolveDependency (JobGroup* group);
	
	JobInfo* Pop (JobGroupID gid);
	//void PushAll (JobGroup* first, JobGroup* last);

	unsigned EnqueueAllInternal (JobGroup* first, JobGroup* last, AtomicQueue* queue, int* pri);
	void EnqueueAll (JobGroup* first, JobGroup* last);
	void MainEnqueueAll (JobGroup* first, JobGroup* last);

	int Steal (JobGroup* group, JobInfo *old, atomic_word tag, int count, bool exec = true);
	static void LinkBatchedJob (JobGroup* grp, JobGroup* prev);

	static void* WorkLoop (void* data);
	
	void ProcessJobs (void* profInfo);

	int CountGroup (JobGroup* group);
	int CountGroups (JobGroup* first, JobGroup* last);

private:

	AtomicStack*	m_Stack;
	AtomicQueue*	m_Queue;
	AtomicQueue*	m_MainQueue;
	JobGroupID		m_AnyJobGroupID;
	
	static AtomicStack*	g_GroupPool;
	static AtomicStack*	g_JobPool;

	unsigned int	m_ThreadCount;
	Thread*	m_Threads;
	CappedSemaphore m_IdleSemaphore;
	volatile int m_PendingJobs;
	volatile int m_ActiveThreads;
	volatile int m_Quit;

	const char* m_QueueName;
	const char* m_WorkerName;
	bool	m_AllowMutexLock;
};

ENUM_FLAGS(JobQueue::JobQueueFlags);

class JobGroup
{
	friend class JobQueue;
	
public:
	// Need comments on when it is safe to be called for every function.

	JobInfo* FirstJob ();
	JobInfo* LastJob ();

	void SetDependency (JobGroupID depend);
	void SetCompletionFunction (JobFunc* complete, void *userData);
	
private:
	JobGroup()
	: count(0)
	, last(NULL)
	, userData(NULL)
	, node(NULL)
	, pri(0)
	{
	}

	int GetJobCountExcludingCompleteJob() const;

	AtomicList	list;
	volatile int count;
	JobInfo*	last;
	JobFunc*	complete;
	void*		userData;
	AtomicNode* node;
	JobGroupID	depend;
	int			pri;
};

void CreateJobQueue (const char* queueName, const char* workerName);
void DestroyJobQueue ();
bool JobQueueCreated ();
EXPORT_COREMODULE JobQueue& GetJobQueue ();

// A convenience function for adding jobs to the main thread queue. In essence, this is
// a shorthand for 
//     GetJobQueue ().ScheduleJob(func, userData, dependency, JobQueue::kMainJobPriority)
// for regular multithreaded queues.
//
// This function is also defined for a single threaded queue, where it simply executes the callback.
// Note that a single threaded job queue does not have kMainJobPriority defined, so this function
// helps to avoid preprocessor clutter.
JobGroupID ScheduleMainThreadJob (JobFunc* func, void* userData, JobGroupID dependency);

#if !UNITY_RELEASE
bool IsExecutingForEachJob();
#endif

#else

#include "SingleThreadedJobQueue.h"

#endif // ENABLE_JOB_SCHEDULER

#endif // JOB_QUEUE_H
