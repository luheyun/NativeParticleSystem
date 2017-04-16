#pragma once
// ------------------------------------------------------
// Batched / chunked parallel for each.
// Provide an array of data and this will kick
// ------------------------------------------------------
#include "Jobs.h"
#include "JobSystem.h"

// Kick off a set of batches
// . Call func on each item in list
// . Calls to func are made in batches from totalBatches job clumps
// . totalBatches = 0 means schedule GetJobQueueThreadCount() num batches.
// . calling this without enough tasks to fit into the batch count specified will
//   result in things being executed immediately on the current thread.
// . Completion of batches is controlled by another schedule attempt or manually calling Sync()
// . If the object goes out of scope it will block as execution depends on the object state.
template<typename TASKT , void (*FUNC)( typename TASKT::value_type& ) >
class BatchedForEach
{

	// Worker thunk that executes a single batch of calls to the task function.
	static void ExecBatch( void * self , unsigned iteration )
	{
		BatchedForEach<TASKT,FUNC> * jb = static_cast<BatchedForEach*>(self);

	#if ENABLE_PROFILER
		ProfilerInformation* profiler = jb->mProfiler;
		if (profiler != NULL) {	PROFILER_BEGIN(*profiler, NULL)	}
	#endif

		int start = iteration*jb->mBatchSize;
		int end   = std::min(jb->mTotalJobs,start + jb->mBatchSize);
		for(int i = start ; i < end ; ++i )
		{
			FUNC( (*(jb->mTasks))[i]  );
		}

	#if ENABLE_PROFILER
		if (profiler != NULL) { PROFILER_END	}
	#endif
	}

	#if ENABLE_PROFILER
	ProfilerInformation*    mProfiler;
	#endif
	TASKT *                 mTasks;
	JobFence                mFence;
	int                     mTotalBatches;
	int                     mBatchSize;
	int                     mTotalJobs;

	bool CalculateBatchStatsAndShouldProcess(int threadCount, int totalBatches, int jobCount)
	{
		mTotalBatches = totalBatches == 0 ? threadCount : totalBatches;
		mTotalJobs    = jobCount;
		mBatchSize    = (mTotalJobs + (mTotalBatches-1)) / mTotalBatches; // round up to include last data.
		return mTotalJobs >= mTotalBatches;
	}

public:
	BatchedForEach(ProfilerInformation* profiler = NULL)
	#if ENABLE_PROFILER
	:	mProfiler (profiler)
	#endif
	{}

	~BatchedForEach()       { Sync();            }

	// Block on all batches completing, or access my fence directly and do what you need with it.
	void       Sync()       { SyncFence(mFence); }
	JobFence & GetFence()   { return mFence;     }

	// Main batch kick off. NOTE: the batcher lifetime is connected to the state
	// so kicking off another batch will block.
	void Schedule (TASKT & list, JobPriority priority=kNormalJobPriority, int totalBatches = 0)
	{
		Sync();

		int threadCount = std::max<int>(GetJobQueueThreadCount(), 1);
		if( CalculateBatchStatsAndShouldProcess(threadCount, totalBatches,list.size()) )
		{
			mTasks                   = &list;
			ScheduleJobForEach( mFence , &BatchedForEach<TASKT,FUNC>::ExecBatch, this, mTotalBatches , NULL, priority );
		}
		else
		{
			for( size_t i = 0; i < list.size(); ++i )
			{
				FUNC( list[i] );
			}
			// Fence is okay here as we should already be in complete state from above.
		}
	}

	void ScheduleAndSync(TASKT& list, int totalBatches = 0)
	{
		Sync();
		
		int threadCount = GetJobQueueThreadCount() + 1;
		if( CalculateBatchStatsAndShouldProcess(threadCount, totalBatches,list.size()) )
		{
			mTasks                   = &list;
			ScheduleJobForEach( mFence , &BatchedForEach<TASKT,FUNC>::ExecBatch, this, mTotalBatches , NULL, kHighJobPriority );
			Sync();
		}
		else
		{
			for( size_t i = 0; i < list.size(); ++i )
			{
				FUNC( list[i] );
			}
			// Fence is okay here as we should already be in complete state from above.
		}
	}

};
