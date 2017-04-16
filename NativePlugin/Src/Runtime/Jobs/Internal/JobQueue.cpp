#include "PluginPrefix.h"
#include "Precomplie/PrefixConfigure.h"
#include "Threads/ThreadToCoreMappings.h"
#include "JobQueue.h"
#include "Utilities/FileStripped.h"
//#include "Misc/AllocatorLabelNames.h"
#include "Misc/AllocatorLabels.h"

#if ENABLE_JOB_SCHEDULER

#include "Threads/Semaphore.h"
#include "Threads/ThreadUtility.h"
#include "Threads/Mutex.h"
#include "Threads/ThreadSpecificValue.h"
#include "Misc/SystemInfo.h"
#include "GfxDevice/GfxDevice.h"


#define SEMAPHORE_WAIT_ON_JOB_THREAD 0//m_Queue->IsEmpty()
#define SEMAPHORE_WAIT_ON_MAIN_THREAD 1
#define EXECUTE_QUEUED_JOB_ON_MAIN_THREAD 0

static UNITY_TLS_VALUE(Semaphore*) g_Semaphore;

//#if !UNITY_RELEASE
//static UNITY_TLS_VALUE(int) g_IsExecutingForEach;
//#endif

/*
@TODO:
* Cleanup atomic operations library. Remove old functions.

* Support for runtime battery consumption control. From script it would be good to have Application.maxWorkerThreadCount
   So that our customers have control over battery consumption.
   (Not all games need multicore, and it can change depending on what the game is doing. Eg when showing a menu, we dont need to run on a thread.)
   If we can do it without overhead for job execution, it would be best if we can change the value while running after startup.

* In the atomic operations we should test for C++11 support and use the builtin compiler intrinsics for atomic operations and
  only revert to inline assembly implementation for legacy compilers.

* In WaitForJobGroup allow for execution of jobs with no dependencies (info->group->depend.info == NULL)

*/

#define	QUEUED_BIT	0x80000000

JobInfo* JobGroup::FirstJob ()
{
	return (JobInfo*) list.Peek ();
}

JobInfo* JobGroup::LastJob ()
{
	return last;
}

void JobGroup::SetCompletionFunction (JobFunc* complete, void *userData)
{
	this->complete = complete;
	this->userData = userData;
}

void JobGroup::SetDependency (JobGroupID depend)
{
	this->depend = depend;
}

int JobGroup::GetJobCountExcludingCompleteJob() const
{
	int res = (count & ~QUEUED_BIT);
	if (complete)
		--res;
	return res;
}

JobQueue::JobQueue (unsigned numthreads, size_t tempAllocSize, int startProcessor, JobQueueFlags flags, const char* queueName, const char* workerName)
:	m_ThreadCount (numthreads)
,	m_IdleSemaphore (numthreads)
,	m_PendingJobs (0)
,	m_ActiveThreads (0)
,   m_Quit (0)
,	m_QueueName(queueName)
,	m_WorkerName(workerName)
{
	m_Stack				= CreateAtomicStack ();
	m_Queue             = CreateAtomicQueue ();
	m_MainQueue             = CreateAtomicQueue ();
	m_AnyJobGroupID		= GetJobGroupID(CreateGroup(0, JobGroupID()));
	m_AllowMutexLock = flags & kAllowMutexLocks;
	if (m_ThreadCount > 0)
	{
		m_Threads = new Thread[numthreads];

		for (int i = 0; i < numthreads; ++i)
		{
			int processor = DEFAULT_UNITY_THREAD_PROCESSOR;
#ifdef JOB_SCHEDULER_THREAD_AFFINITY_MASK
			processor = JOB_SCHEDULER_THREAD_AFFINITY_MASK;
#else
			if (startProcessor >= 0)
			{
				// This is a mask not a id so build the mask.
				processor = (1 << (startProcessor + i));
			}
#endif // JOB_SCHEDULER_THREAD_AFFINITY_MASK
			m_Threads[i].SetName (workerName);
			m_Threads[i].SetTempAllocatorSize(tempAllocSize);
			m_Threads[i].Run (&WorkLoop, this, DEFAULT_UNITY_THREAD_STACK_SIZE, processor);
		}
	}
	else
	{
		m_Threads = NULL;
	}
#if JOB_QUEUE_REQUIRES_PLATFORM_THREAD_AFFINITY_CONTROL
	for (int i = 0; i < m_ThreadCount; ++i)
	{
		//	Provide an impl for SetAffinityForJobQueueWorkerIndex in PlatformThread to use this
		m_Threads[i].GetThread().SetAffinityForJobQueueWorkerIndex (i);
	}
#endif
}

JobQueue::~JobQueue ()
{
	Shutdown(kShutdownImmediate);
}

void JobQueue::Shutdown(ShutdownMode shutdownMode)
{
	unsigned i;
	if (m_Quit != 0)
		return;

	m_Quit = shutdownMode;
	for (i = 0; i < m_ThreadCount; ++i)
	{
		m_IdleSemaphore.Signal ();
	}
	
	// wait for each thread to exit
	for (i = 0; i < m_ThreadCount; ++i)
	{
		m_Threads[i].WaitForExit ();
	}
	delete[] m_Threads;

	while (ExecuteOneJobOnMainThread()) {};

	g_GroupPool->Push ((AtomicNode*) m_AnyJobGroupID.info->node);
	
	// cleanup node pool
	AtomicNode* node = g_GroupPool->PopAll ();
	while (node)
	{
		AtomicNode* next = node->Next ();
		JobGroup* group = (JobGroup*) node->data[0];
		group->~JobGroup();
		UNITY_FREE (kMemThread, group);
		UNITY_FREE (kMemThread, node);
		node = next;
	}

	JobInfo* info = (JobInfo*) g_JobPool->PopAll ();
	while (info)
	{
		JobInfo* next = info->Next ();
		UNITY_FREE (kMemThread, info);
		info = next;
	}

	DestroyAtomicQueue (m_Queue);
	DestroyAtomicStack (m_Stack);
	DestroyAtomicQueue (m_MainQueue);
}

JobInfo* JobQueue::Pop (JobGroupID gid)
{
	JobInfo* info = (JobInfo*) m_Stack->Pop ();
	
	return info;
}

// should only be called after a successful Clear with <first, last>
void JobQueue::ScheduleDependencies (JobGroup* group, JobInfo *first, JobInfo *last)
{
	JobGroupID j, i = group->depend;
	atomic_word tag;
	
	// push all dependencies on the stack
	while(i.info && i.info != m_AnyJobGroupID.info)
	{
		j = i.info->depend;
		JobInfo* info = (JobInfo*) i.info->list.Load (tag);
		
		if (tag == i.version - 2)
		{
			JobInfo *depend_first = (JobInfo*) i.info->list.Clear ((AtomicNode*) info, tag);
			// j is still valid
			if (depend_first)
			{
				// state is set to 'pushed', we now own it
				JobInfo* depend_last = i.info->last;

				int c = i.info->list.Add ((AtomicNode*) first, (AtomicNode*) last, tag + 1);
				first = depend_first;
				last = depend_last;
				
				// j is valid, capture next dependency and loop again
				i = j;
			}
			else
			{
				// someone else stole the dependency, try deferring previous list
				if (i.info->list.Add ((AtomicNode*) first, (AtomicNode*) last, tag + 1))
				{
					// success!
					first = last = NULL;
				}
				else
				{
					// the dependency has actually finished, break and push list
				}
				break;
			}
		}
		else if(tag == i.version - 1)
		{
			// j is still valid and is in the process of being pushed
			
			// try defering previous list
			if (i.info->list.Add ((AtomicNode*) first, (AtomicNode*) last, tag))
			{
				// success!
				first = last = NULL;
			}
			else
			{
				// the dependency has actually finished, push to the execution stack and return
				// we can't verify anything at this point, the JobGroup could have been reused
				// AssertMsg(i.info->list.Tag() != tag, "Incorrect tag");
			}
			break;
		}
		else
		{
			// j is not valid and i is done
			break;
		}
	}
	if(first)
	{
		// no more dependencies, push to execution stack and return
		m_Stack->PushAll ((AtomicNode*) first, (AtomicNode*) last);
	}
}

void JobQueue::ResolveDependency (JobGroup* group)
{
	// resolve dependencies
	atomic_word tag;
	JobInfo* old = (JobInfo*) group->depend.info->list.Load (tag);

	if (tag == group->depend.version - 2)
	{
		// the group is still in the queue, try to steal it
		if (Steal (group->depend.info, old, tag, 1) > 0)
		{
			return;
		}
		
		// refetch tag
		old = (JobInfo*) group->depend.info->list.Load (tag);
	}
	while (tag == group->depend.version - 1)
	{
		// the group is in the execution stack, pop until it's done
		JobInfo* info = Pop (group->depend);
		if (info)
		{
			int same = info->group == group->depend.info;
			
			if ((Exec (info, info->group->list.Tag () + 1, 1) > 0) && same)
			{
				return;
			}
		}
		else
		{
			AtomicList::Relax ();
		}
		// refetch tag
		group->depend.info->list.Load (tag);
	}
}

inline void JobQueue::ExecuteJobFunc (JobInfo* info)
{
	if (info->isForEach)
	{
		JobForEachFunc* jobFunction = (JobForEachFunc*) info->jobFunction;
		(*jobFunction) (info->userData, info->forEachIndex);
	}
	else
	{
		(*info->jobFunction) (info->userData);
	}
}

static void SignalSemaphore(void *sema)
{
	((Semaphore *) sema)->Signal();
}

int JobQueue::Exec (JobInfo* info, atomic_word tag, int count)
{
	// note: at this point, group->node is volatile!
	AtomicDecrement(&m_PendingJobs);


	int res = 0;
	
	JobGroup* group = info->group;
	JobFunc* complete = group->complete;
	void* userData = group->userData;

	if (group->depend.info && group->depend.info != m_AnyJobGroupID.info)
	{
		ResolveDependency (group);
	}
	
	ExecuteJobFunc (info);
	
	int n = AtomicSub (&group->count, count);
	int remaining = n & (QUEUED_BIT - 1);

	if (complete && remaining == 1)
	{
		// last job of this group
		(*complete) (userData);
		n = AtomicDecrement (&group->count);
		--remaining;
	}

	if (remaining == 0)
	{
		// set state to 'done', and retrieve nodes to be rescheduled
		AtomicNode *first = group->list.Touch (tag);
		if (first)
		{
			JobInfo *current = (JobInfo *) first;
			AtomicNode *last = NULL;
			first = NULL;
			JobInfo *sfirst = NULL, *slast = NULL;
			int count = 0;
			
			while (current)
			{
				JobInfo *next = current->Next ();
				
				if (current->jobFunction == &SignalSemaphore)
				{
					// shortcut for semaphores: call immediately and avoid going through the stack
					(*current->jobFunction)(current->userData);
					if (!sfirst)
					{
						sfirst = slast = current;
					}
					else
					{
						slast->Link(current);
						slast = current;
					}
				}
				else
				{
					if (!first)
					{
						first = last = (AtomicNode*) current;
					}
					else
					{
						last->Link((AtomicNode*) current);
						last = (AtomicNode*) current;
					}
					++count;
				}
				current = next;
			}
			
			if (first)
			{
				m_Stack->PushAll (first, last);

				Wake(count);
			}
			if (sfirst)
			{
				g_JobPool->PushAll((AtomicNode*) sfirst, (AtomicNode*) slast);
			}
		}

		if (n == 0)
		{
			// note: at this point, we can safely refer to group->node
			AtomicNode *node = group->node;
			
			// only release group if both queued bit and count are 0
			g_GroupPool->Push (node);
		}
		res = 1;
	}
	// release job
	g_JobPool->Push ((AtomicNode*) info);
	return res;
}

//	Note that this function can be called concurrently and from the work loop itself
void JobQueue::Wake (unsigned n)
{
	//	Attempt to wake up as many threads as required or the number that are idle
	n = std::min (n,m_ThreadCount);

	m_IdleSemaphore.Signal (n);
}

int JobQueue::Steal (JobGroup* group, JobInfo* info, atomic_word tag, int count, bool exec)
{
	int res = 1;

	// try to empty the list
	JobInfo *next = (JobInfo*) group->list.Clear ((AtomicNode*) info, tag);
	bool recycle = next ? false : true;
	if (next)
	{
		// state is set to 'pushed'
		JobInfo* last = group->last;

		// must ALWAYS schedule dependencies before pushing to the stack
		if (group->depend.info && group->depend.info != m_AnyJobGroupID.info)
		{
			ScheduleDependencies (group, info, last);
			res = 0;
			recycle = true;
		}
		else
		{
			JobInfo* first = NULL;
			if (exec)
			{
				if (info != last)
				{
					first = info->Next ();
					m_Stack->PushAll ((AtomicNode*) first, (AtomicNode*) last);

					res = -1;
				}
				// execute first task
				Exec (info, tag + 2, count);
			}
			else
			{
				m_Stack->PushAll ((AtomicNode*) first, (AtomicNode*) last);
				res = -1;
			}
		}
	}
	
	if (recycle)
	{
		if(count & QUEUED_BIT)
		{
			if(AtomicSub(&group->count, QUEUED_BIT) == 0)
			{
				// wait until the completion function is done
				while (1)
				{
					group->list.Load (tag);
					if(tag & 1)
					{
						AtomicList::Relax ();
					}
					else
					{
						break;
					}
				}

				// group is now safe to be released
				g_GroupPool->Push ((AtomicNode*) group->node);
			}
		}

		res = 0;
	}

	return res;
}

/*
	PushAll (High priority stack scheduled via JobBatchDispatch) Codepath currently not supported
	because can lead to invalid job group situation.
	Talk to benoit if you want to add support for this.

void JobQueue::PushAll (JobGroup* first, JobGroup* last)
{
	AtomicNode* curr = first->node;
	JobInfo* firstJob = NULL;
	JobInfo* lastJob = NULL;
	unsigned n = 0;
	
	while (curr)
	{
		JobGroup* group = (JobGroup*) curr->data[0];
		
		group->pri = kHighJobPriority;
		
		atomic_word tag;
		JobInfo* info = (JobInfo*) group->list.Load (tag);
		
		AssertMsg ((group->count & QUEUED_BIT) == 0, "Queued group in PushAll!");
		AssertMsg ((tag & 1) == 0, "Odd tag in PushAll!");

		// link jobs together
		if (lastJob)
		{
			lastJob->Link (info);
		}
		else
		{
			firstJob = info;
		}
		lastJob = group->last;

		n += group->count;

		// set state to 'pushed'
		group->list.Reset (NULL, tag + 1);

		if (group == last)
		{
			break;
		}
		else
		{
			curr = curr->Next ();
		}
	}
	m_Stack->PushAll ((AtomicNode*) firstJob, (AtomicNode*) lastJob);

	Wake (n);
}
*/


unsigned JobQueue::EnqueueAllInternal (JobGroup* first, JobGroup* last, AtomicQueue* queue, int* pri)
{
	JobGroup* group = first;
	unsigned n = 0;
	
	while (group)
	{
		if (pri) group->pri = *pri;

		// These asserts can not be used in the EnqueueAll codepath because QUEUED_BIT is already set in the creation
		// in order to support SyncFence for created but not yet Enqueued jobs
		//AssertMsg ((group->count & QUEUED_BIT) == 0, "Queued group in EnqueueAll!");
		//AssertMsg ((group->list.Tag () & 1) == 0, "Odd tag in EnqueueAll!");

		// queued bit should already be set by CreateJobBatch()
		n += group->GetJobCountExcludingCompleteJob();

		if (group == last)
		{
			break;
		}
		AtomicNode *node = group->node->Next();
		if(node)
		{
			group = (JobGroup *) node->data[0];
		}
		else
		{
			break;
		}
	}

	AtomicAdd(&m_PendingJobs, n);
	
	queue->EnqueueAll (first->node, last->node);

	return n;
}

// only to be called with groups created by CreateJobBatch()
void JobQueue::MainEnqueueAll (JobGroup* first, JobGroup* last)
{
	int pri = kMainJobPriority;
	EnqueueAllInternal (first, last, m_MainQueue, &pri);
}

void JobQueue::EnqueueAll (JobGroup* first, JobGroup* last)
{
	unsigned n = EnqueueAllInternal (first, last, m_Queue, NULL);

	Wake (n);
}

bool JobQueue::ExecuteOneJob ()
{
	if (ExecuteJobFromHighPriorityStack ())
		return true;
	else
		return ExecuteJobFromQueue();
}

bool JobQueue::ExecuteJobFromHighPriorityStack ()
{
	JobInfo* info = (JobInfo*) m_Stack->Pop ();
	if (info)
	{
		atomic_word tag = info->group->list.Tag ();
		
		Exec (info, tag + 1, 1);
		return true;
	}
	else
		return false;
}

bool JobQueue::ExecuteJobFromQueue ()
{
	bool result = false;

	AtomicNode* node = m_Queue->Dequeue ();
	// then try to dequeue from the queue
	if (node != NULL)
	{
		JobGroup* group = (JobGroup*) node->data[0];
		
		// group nodes are volatile, so reassign it
		group->node = node;
		
		atomic_word tag;
		JobInfo* job = (JobInfo*) group->list.Load (tag);
		
		// verify tag as well! an odd <tag> would indicate that <job> does not belong to this group...
		if (job && (tag & 1) == 0)
		{
			// Exec () will clear the queued bit
			Steal (group, job, tag, QUEUED_BIT + 1);
		}
		else if (AtomicSub (&group->count, QUEUED_BIT) == 0)
		{
			// should no longer be needed with completion function incrementing count
			// wait until the completion function is done
			while (1)
			{
				group->list.Load (tag);
				if(tag & 1)
				{
					AtomicList::Relax ();
				}
				else
				{
					break;
				}
			}

			// group is now safe to be released
			g_GroupPool->Push ((AtomicNode*) node);
		}

		result = true;
	}
	
	return result;
}

int JobQueue::ExecuteJobFromMainQueue ()
{
	int count = 0;
	
	AtomicNode* node = m_MainQueue->Dequeue ();
	// then try to dequeue from the queue
	if (node != NULL)
	{
		JobGroup* group = (JobGroup*) node->data[0];
		
		// group nodes are volatile, so reassign it
		group->node = node;
		
		atomic_word tag;
		JobInfo* job = (JobInfo*) group->list.Load (tag);
		// make sure list is cleared before execution
		group->list.Reset(NULL, tag);
		
		// Exec () will clear the queued bit
		Exec (job, tag + 2, QUEUED_BIT + 1);
		
		++count;
	}
	return count;
}

void JobQueue::ProcessJobs (void* profInfo)
{
	Mutex::AutoAllowWorkerThreadLock autoAllow(m_AllowMutexLock);

	AtomicIncrement(&m_ActiveThreads);

	while (m_Quit != kShutdownImmediate)
	{
	
	#if ENABLE_JOB_SCHEDULER_PROFILER

		if (profInfo != NULL)
			HandleProfilerFrames ((WorkerProfilerInfo*)profInfo, m_AllowMutexLock);

	#endif

		// try first to pop from the execution stack
		if (ExecuteJobFromHighPriorityStack ())
			;
		// then try to dequeue from the queue
		else if (ExecuteJobFromQueue ())
			;
		else
		{
			if (m_Quit == kShutdownWaitForAllJobs)
			{
				AtomicDecrement(&m_ActiveThreads);
				return;
			}

			// go idle if the number of pending jobs is less than the number of active threads
			if (m_PendingJobs < m_ActiveThreads)
			{
				// go idle and wait for next job(s)
				AtomicDecrement(&m_ActiveThreads);
				m_IdleSemaphore.WaitForSignal ();
				AtomicIncrement(&m_ActiveThreads);
			}
			else
			{
				// yield and try again, there are still jobs waiting
				Thread::Sleep (0.0f);
			}
		}
	}

	AtomicDecrement(&m_ActiveThreads);
}

void* JobQueue::WorkLoop (void* data)
{
	// TODO: we could chain together recycled jobs and postpone deallocation until the queue is empty
	JobQueue* q = (JobQueue*) data;

#if ENABLE_JOB_SCHEDULER_PROFILER
	WorkerProfilerInfo* profInfo = NULL;

	if (q->m_ProfInfo != NULL)
	{
		int workThreadIndex = AtomicIncrement (&q->m_WorkerThreadCounter);
		profInfo = &q->m_ProfInfo[workThreadIndex];
		profInfo->endFrameID = -1; // Mark that the frame hasn't ended yet

		// Start profiling new frame
		profiler_initialize_thread (q->m_QueueName, q->m_WorkerName);
		profiler_begin_frame_separate_thread (kProfilerGame);
		profiler_set_active_separate_thread (true);
	}
#else
	void* profInfo = NULL;
#endif

	q->ProcessJobs (profInfo);

#if ENABLE_JOB_SCHEDULER_PROFILER
	// scope
	{
		if (profInfo != NULL)
		{
			//@TODO: Is this actually necessary? (It seems like cleanup thread should take care of killing it all?)
			profiler_set_active_separate_thread (false);
			profiler_end_frame_separate_thread (0);

			profiler_cleanup_thread ();
		}
	}

#endif

	// exit thread
	return NULL;
}

#if ENABLE_JOB_SCHEDULER_PROFILER

void JobQueue::HandleProfilerFrames (WorkerProfilerInfo* profInfo, bool allowMutexLock)
{
	Mutex::AutoAllowWorkerThreadLock autoAllow(allowMutexLock);

	// Don't do fancy synchnonization here; all we need is for worker
	// threads to do begin/end profiler frame once in a while.
	// Worst case, we'll get a missing profiler info for a frame.
	int endFrameID = AtomicExchange(&profInfo->endFrameID, -1);
	if (endFrameID >= 0)
	{
		// Finish profiling old frame
		profiler_set_active_separate_thread (false);
		profiler_end_frame_separate_thread (endFrameID);

		// Start profiling new frame
		profiler_begin_frame_separate_thread (kProfilerGame);
		profiler_set_active_separate_thread (true);
	}
}

#endif

JobGroup* JobQueue::CreateGroup (unsigned jobs, JobGroupID depends)
{
	JobGroup* group = NULL;
	
	AtomicNode* node = g_GroupPool->Pop ();

	if (node == NULL)
	{
		node = (AtomicNode*) UNITY_MALLOC_ALIGNED(kMemThread, sizeof(AtomicNode), 16);
		group = (JobGroup*) UNITY_MALLOC_ALIGNED (kMemThread, sizeof (JobGroup), 16);
		node->data[0] = group;
		node->data[1] = NULL;
		node->data[2] = NULL;
		new (group) JobGroup();
		group->list.Init ();
	}
	else
	{
		group = (JobGroup*) node->data[0];

		node->data[1] = NULL;
		node->data[2] = NULL;
	}
	group->node = node;
	node->Link (NULL);

	if (jobs != 0)
	{
		JobInfo* first = NULL;
		JobInfo* last = NULL;
		for (unsigned i = 0; i < jobs; ++i)
		{
			JobInfo* info = (JobInfo*) g_JobPool->Pop ();
			if (!info)
			{
				info = (JobInfo*) UNITY_MALLOC_ALIGNED(kMemThread, sizeof(JobInfo), 16);
			}
			info->group = group;
			
			if (last)
			{
				last->Link (info);
			}
			else
			{
				first = info;
			}
			last = info;
		}
		last->Link (NULL);
		group->last = last;
		group->count = jobs;
		group->complete = NULL;
		group->userData = NULL;
		group->depend = depends;
		
		group->list.Reset ((AtomicNode*) first, group->list.Tag ());
	}
	else
	{
		group->last = NULL;
		group->count = 0;
		group->complete = NULL;
		group->userData = NULL;
		group->depend = depends;
	}
	return group;
}

inline void JobQueue::LinkBatchedJob (JobGroup* grp, JobGroup* prev)
{
	if (prev)
		prev->node->Link (grp->node);
	grp->count |= QUEUED_BIT;
}

JobGroup* JobQueue::CreateJobBatch (JobFunc* func, void* userData, JobGroupID dependency, JobGroup* prev)
{
	JobGroup* grp = CreateJob(func, userData, dependency);
	LinkBatchedJob (grp, prev);
	return grp;
}

JobGroup* JobQueue::CreateForEachJobBatch (JobForEachFunc* func, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobGroup* prev)
{
	JobGroup* grp = CreateJobsForEach (func, userData, n, complete, dependency);
	LinkBatchedJob (grp, prev);
	return grp;
}

JobGroupID JobQueue::GetJobGroupID (JobGroup* group)
{
	JobGroupID gid;
	
	gid.info = group;
	gid.version = group->list.Tag () + 2;
	
	return gid;
}

int JobQueue::CountGroup (JobGroup* group)
{
	int n = 0;
	JobInfo* info = group->FirstJob ();
	JobInfo* last = group->LastJob ();
	
	while (info)
	{
		++n;
		if(info == last)
		{
			break;
		}
		info = info->Next ();
	}
	
	// Complete function is counted as a job
	if (group->complete)
		++n;

	return n;
}

int JobQueue::CountGroups (JobGroup* first, JobGroup* last)
{
	int n = 0;
	
	while (first)
	{
		++n;

		if (first == last)
		{
			break;
		}
		AtomicNode *node = first->node->Next();
		first = node ? (JobGroup *) node->data[0] : NULL;
	}

	return n;
}

JobGroupID JobQueue::ScheduleGroup (JobGroup* group, JobQueuePriority priority)
{
	JobInfo* info = (JobInfo*) group->list.Peek ();
	JobGroupID gid;
	
	if (info)
	{
		gid = GetJobGroupID (group);

		unsigned n = group->GetJobCountExcludingCompleteJob();
		AtomicAdd(&m_PendingJobs, n);
		
		group->pri = priority;
		if (priority == kMainJobPriority)
		{
			group->count |= QUEUED_BIT;
			m_MainQueue->Enqueue (group->node);
			
			return gid;
		}
		else if (priority == kHighJobPriority)
		{
			atomic_word tag;
			JobInfo* first = (JobInfo*) group->list.Load (tag);
			JobInfo* last = group->last;

			// set state to 'pushed'
			group->list.Reset (NULL, tag + 1);
			
			// must ALWAYS schedule dependencies before pushing to the stack
			if (group->depend.info && group->depend.info!=m_AnyJobGroupID.info)
			{
				ScheduleDependencies (group, first, last);
			}
			else
			{
				m_Stack->PushAll ((AtomicNode*) first, (AtomicNode*) last);
			}
		}
		else
		{
			group->count |= QUEUED_BIT;
			m_Queue->Enqueue (group->node);
		}
	
		Wake (n);
	}
	return gid;
}

void JobQueue::ScheduleGroups (JobGroup* first, JobGroup* last)
{
	EnqueueAll (first, last);
}

JobGroup* JobQueue::CreateJob (JobFunc* func, void* userData, JobGroupID dependency)
{
	JobGroup* group = CreateGroup (1, dependency);
	
	group->FirstJob ()->Set (func, userData);

	return group;
}

JobGroupID JobQueue::ScheduleJob (JobFunc* func, void* userData, JobGroupID dependency, JobQueuePriority priority)
{
	return ScheduleGroup (CreateJob (func, userData, dependency), priority);
}

JobGroupID JobQueue::ScheduleJobsForEach (JobForEachFunc* jobFunction, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency, JobQueuePriority priority)
{
	return ScheduleGroup(CreateJobsForEach (jobFunction, userData, n, complete, dependency), priority);
}

JobGroup* JobQueue::CreateJobsForEach (JobForEachFunc* jobFunction, void* userData, unsigned n, JobFunc* complete, JobGroupID dependency)
{
	JobGroup* group = CreateGroup (n, dependency);
	JobInfo* info = group->FirstJob ();
	
	for (unsigned i = 0; i < n; ++i)
	{
		info->jobFunction = (JobFunc*) jobFunction;
		info->userData = userData;
		info->isForEach = 1;
		info->forEachIndex = i;
		info = (JobInfo*) info->Next ();
	}
	group->complete = complete;
	group->userData = userData;

	if (complete)
	{
		// Complete function is counted as a job
		group->count++;
	}
	return group;
}

struct WaitForJobGroupProfiler
{
#if ENABLE_JOB_SCHEDULER_PROFILER
	bool didBegin;
#endif
	inline WaitForJobGroupProfiler ()
	{
		#if ENABLE_JOB_SCHEDULER_PROFILER
		didBegin = false;
		#endif
	}

	inline void Begin ()
	{
		#if ENABLE_JOB_SCHEDULER_PROFILER
		if (!didBegin)
		{
			didBegin = true;
			PROFILER_BEGIN(gWaitForJob, NULL);
		}
		#endif
	}
	inline ~WaitForJobGroupProfiler ()
	{
		#if ENABLE_JOB_SCHEDULER_PROFILER
		if (didBegin)
		{
			PROFILER_END
		}
		#endif
	}
};

void JobQueue::Cleanup()
{
	// if no worker threads are running, dequeue one node
	AtomicNode *node;
	
	if ((node = m_Queue->Dequeue ()))
	{
		JobGroup* group = (JobGroup*) node->data[0];
		
		// group nodes are volatile, so reassign it
		group->node = node;
		
		atomic_word tag;
		JobInfo* first = (JobInfo*) group->list.Load (tag);
		
		if (first)
		{
			// try to empty the list
			JobInfo *next = (JobInfo*) group->list.Clear ((AtomicNode*) first, tag);
			if (next)
			{
				// state is set to 'pushed'
				JobInfo* last = group->last;
				
				// put all jobs on the execution stack
				m_Stack->PushAll ((AtomicNode*) first, (AtomicNode*) last);
			}
		}
		if (AtomicSub (&group->count, QUEUED_BIT) == 0)
		{
			g_GroupPool->Push ((AtomicNode*) node);
		}
	}
}

// when calling SignalOnFinish with always=false, then one must check the return value to verify
// if the semaphore will be signaled:
//
//		if (SignalOnFinish(gid, sema, false))
//			sema->WaitForSignal();
//
// when calling SignalOnFinish with always=true, then the semaphore will be signaled even if the
// group is finished, so it's safe to do:
//
//		SignalOnFinish(gid, sema, true);
//		...
//		sema->WaitForSignal();
//
//	this form is useful when the waiter has no way to know the return value of SignalOnFinish()
//	but we must guaranty that the semaphore is signaled as if it was done by the finishing group.

int JobQueue::SignalOnFinish(JobGroupID gid, Semaphore *sema, bool always)
{
	JobGroup* grp = (JobGroup*) gid.info;
	
	if (grp)
	{
		atomic_word tag;
		JobInfo* old = (JobInfo*) grp->list.Load (tag);

		if (tag == gid.version - 2)
		{
			if (Steal (grp, old, tag, 1, false) > 0)
			{
				if (m_ThreadCount == 0)
					Cleanup();

				if (always)
				{
					sema->Signal ();
				}
				return 0;
			}

			old = (JobInfo*) grp->list.Load (tag);
			
		}
		if (tag == gid.version - 1)
		{
			JobInfo* info = (JobInfo*) g_JobPool->Pop ();
			if (!info)
			{
				info = (JobInfo*) UNITY_MALLOC_ALIGNED(kMemThread, sizeof(JobInfo), 16);
			}
			info->jobFunction = &SignalSemaphore;
			info->userData = sema;
			info->isForEach = 0;
			info->forEachIndex = 0;
			info->group = NULL;
			
			// try defering semaphore signal
			if (grp->list.Add ((AtomicNode*) info, (AtomicNode*) info, tag))
			{
				// success! semaphore will be signaled
				return 1;
			}
			else
			{
				// the group has actually finished, recycle job info
				g_JobPool->Push ((AtomicNode*) info);
			}
		}
	}
	if (always)
	{
		sema->Signal ();
	}
	return 0;
}

void JobQueue::WaitForJobGroup (JobGroupID gid, bool execMain)
{
	WaitForJobGroupProfiler waitProfiler;
	bool main = Thread::CurrentThreadIsMainThread();
	
	JobGroup* grp = (JobGroup*) gid.info;
	JobInfo* info;
	
	if (grp)
	{
		int pri = grp->pri; // must be fetched before tag!
		atomic_word tag;
		JobInfo* old = (JobInfo*) grp->list.Load (tag);

		if (((tag == gid.version - 2) || (tag == gid.version - 1)) && pri==kMainJobPriority)
		{
			while ((tag == gid.version - 2) || (tag == gid.version - 1))
			{
				waitProfiler.Begin();

				if (main)
				{
					// only pick jobs from the main queue
					ExecuteJobFromMainQueue();
				}
				else
				{
					info = Pop (gid);
					if (info)
					{
						int same = info->group == grp;
						
						if ((Exec (info, info->group->list.Tag () + 1, 1) > 0) && same)
						{
							if (m_ThreadCount == 0)
								Cleanup();

							return;
						}
					}
					else
					{
						AtomicList::Relax ();
					}
				}
				old = (JobInfo*) grp->list.Load (tag);
			}
		}
		else
		{
			if (tag == gid.version - 2)
			{
				waitProfiler.Begin();
				if (Steal (grp, old, tag, 1) > 0)
				{
					if (m_ThreadCount == 0)
						Cleanup();

					return;
				}

				old = (JobInfo*) grp->list.Load (tag);
			}

			while (tag == gid.version - 1)
			{
				waitProfiler.Begin();

				if(main && execMain)
				{
					ExecuteJobFromMainQueue();
				}
				
				info = Pop (gid);
				if (info)
				{
					int same = info->group == grp;
					
					if ((Exec (info, info->group->list.Tag () + 1, 1) > 0) && same)
					{
						if (m_ThreadCount == 0)
							Cleanup();

						return;
					}
				}
				else
				{
					if ((SEMAPHORE_WAIT_ON_JOB_THREAD && !main) || (SEMAPHORE_WAIT_ON_MAIN_THREAD && main))
					{
						Semaphore* sema = g_Semaphore;
						if (sema == NULL)
						{
							g_Semaphore = sema = UNITY_NEW(Semaphore, kMemThread);
						}
					
						if (SignalOnFinish (gid, sema, false))
						{
							// conditionally wait on semaphore (passing false will return non-zero only if
							// a job was appended to the group's list).
							sema->WaitForSignal();
						}
						else
						{
							// if the group has already finished, then SignalOnFinish(false) won't signal
							// the semaphore, so there is no need to wait on it
						}
					}
					else
					{
						AtomicList::Relax ();
					}
				}
				old = (JobInfo*) grp->list.Load (tag);
			}
		}
	}

	if (m_ThreadCount == 0)
		Cleanup();
}

bool JobQueue::ExecuteOneJobOnMainThread()
{
	//AssertMsg(Thread::CurrentThreadIsMainThread() != 0, "Not main thread");
	return ExecuteJobFromMainQueue() != 0;
}

bool JobQueue::HasJobGroupCompleted (JobGroupID group)
{
	JobGroup* grp = (JobGroup*) group.info;
	atomic_word tag;
	
	if (grp)
	{
		tag = grp->list.Tag ();

		return tag != (group.version - 2) && tag != (group.version - 1);
	}
	else
	{
		return true;
	}
}

void JobQueue::SetThreadPriority (int priority)
{
	for (int i = 0; i < m_ThreadCount; ++i)
		m_Threads[i].SetPriority ((ThreadPriority)priority);
}


static JobQueue* g_Queue = NULL;

AtomicStack* JobQueue::g_GroupPool = NULL;
AtomicStack* JobQueue::g_JobPool = NULL;

void CreateJobQueue (const char* queueName, const char* workerName)
{
	// those would be shared across schedulers
	JobQueue::g_GroupPool	= CreateAtomicStack ();
	JobQueue::g_JobPool	= CreateAtomicStack ();

#if UNITY_OSX
	Thread::SetCurrentThreadProcessor (0);
#endif // UNITY_OSX

	int startProcessor = JOB_SCHEDULER_START_PROCESSOR;
	int workerThreads = JOB_SCHEDULER_MAX_THREADS;

	// Don't use an unreasonable amount of threads on future hardware.
	// Mono GC has a 256 thread limit for the process (case 443576).
	if (workerThreads > 128)
		workerThreads = 128;
	
	//@TODO: For now we allow mutex locks on worker threads. This is a performance problem, but we still have too many before we can enable this.
	//       It is still useful for enabling it temporarily to detect specific issues in a subsystem.
	g_Queue = new JobQueue (workerThreads, 256*1024, startProcessor, JobQueue::kUseProfiler | JobQueue::kAllowMutexLocks, queueName, workerName);
}

void DestroyJobQueue ()
{
	delete g_Queue;
	g_Queue = NULL;

	// those would be shared across schedulers
	DestroyAtomicStack (JobQueue::g_JobPool);
	DestroyAtomicStack (JobQueue::g_GroupPool);
}

bool JobQueueCreated ()
{
	return (g_Queue != NULL);
}

JobQueue& GetJobQueue ()
{
	return *g_Queue;
}

JobGroupID ScheduleMainThreadJob (JobFunc* func, void* userData, JobGroupID dependency)
{
	JobQueue& queue = GetJobQueue ();
	return queue.ScheduleJob(func, userData, dependency, JobQueue::kMainJobPriority);
}

#endif // ENABLE_JOB_SCHEDULER
