#include "UnityPrefix.h"

#if ENABLE_UNIT_TESTS

#include "Configuration/UnityConfigure.h"
#include "Runtime/Testing/Testing.h"
#include "Runtime/Testing/TestFixtures.h"
#include "Runtime/Jobs/Internal/JobQueue.h"
#include "Runtime/Jobs/Jobs.h"

SUITE (JobQueueTests)
{
	class StubJob
	{
	public:
		UInt32 m_JobInvoked;

		static void MyJobFunc(void* userData)
		{
			StubJob* pStubJob = (StubJob*)userData;
			pStubJob->m_JobInvoked++;
		}

		StubJob()
		: m_JobInvoked(0)
		{}
	};

	class JobQueueFixture : TestFixtureBase
	{
	public:
		JobQueueFixture()
		: m_StubJob(NULL), m_JobQueue(NULL)
		{}

		void Initialize()
		{
			m_StubJob = UNITY_NEW(StubJob, kMemDefault);
			m_JobQueue = UNITY_NEW(JobQueue (1, 32*1024, -1, JobQueue::kUseProfilerAndAllowMutexLocks, "TestGroup", "kJobQueueTypeName"), kMemDefault);
		}

		void ShutdownJobQueue(JobQueue::ShutdownMode shutdownMode)
		{
			m_JobQueue->Shutdown(shutdownMode);
			UNITY_DELETE(m_JobQueue, kMemDefault);
			m_JobQueue = NULL;
		}

		void DeleteJob()
		{
			UNITY_DELETE(m_StubJob, kMemDefault);
			m_StubJob = NULL;
		}

		void ScheduleJob()
		{
			m_JobQueue->ScheduleJob(StubJob::MyJobFunc, m_StubJob, m_JobQueue->GetAnyJobGroupID(), JobQueue::kNormalJobPriority);
		}

		StubJob* m_StubJob;
		JobQueue* m_JobQueue;
	};

/*
	TEST_FIXTURE (JobQueueFixture, JobQueue_NormalQuitModeWithOneJob_CheckNoJobExecuted)
	{
		Initialize();
		m_JobQueue->SetThreadPriority(kBelowNormalPriority);
		ScheduleJob();
		// there is no guaranty that existing jobs will be executed at all when using kShutdownImmediate
		ShutdownJobQueue(JobQueue::kShutdownImmediate);
#if ENABLE_JOB_SCHEDULER
		CHECK_EQUAL(m_StubJob->m_JobInvoked, 0);
#else
		CHECK_EQUAL(m_StubJob->m_JobInvoked, 1);
#endif
		DeleteJob();
	}
*/

	TEST_FIXTURE (JobQueueFixture, JobQueue_WaitForAllBeforeQuitModeWithOneJob_CheckJobExecuted)
	{
		Initialize();
		m_JobQueue->SetThreadPriority(kBelowNormalPriority);
		ScheduleJob();
		ShutdownJobQueue(JobQueue::kShutdownWaitForAllJobs);
		CHECK_EQUAL(m_StubJob->m_JobInvoked, 1);
		DeleteJob();
	}
}

// Tests for the public job API using the normal, global job queue
namespace
{
	struct DependentJob
	{
		enum { kMaxDependencies = 2 };
		JobFence dependencies[kMaxDependencies];
		int* inputs[2];
		int* output;
		volatile bool started;
	};

	void DependentAdd(DependentJob* job)
	{
		job->started = true;
		for (int i = 0; i < DependentJob::kMaxDependencies; ++i)
			SyncFence(job->dependencies[i]);

		*job->output = *job->inputs[0] + *job->inputs[1];
	}

	struct BubbleSortJob
	{
		int count;
		int* input;
		int* output;
		volatile bool started;
	};

	void BubbleSortFindSmallest(BubbleSortJob* job)
	{
		job->started = true;
		int* values = job->input;
		bool stillSorting = true;
		while (stillSorting)
		{
			stillSorting = false;
			for (int i = 0; i < job->count - 1; ++i)
			{
				if (values[i] > values[i+1])
				{
					std::swap(values[i], values[i+1]);
					stillSorting = true;
				}
			}
		}
		*job->output = values[0];
	}

	enum JobsTestFlags
	{
		kJobsTestDefaultFlags = 0,
		kJobsTestMixSyncFenceAndDepends = 1 << 0,
		kJobsTestMixPriorities = 1 << 1,
	};
	ENUM_FLAGS(JobsTestFlags);

	template <int kJobsPerChain, int kSimultaneousChains>
	void TestLongDependencyChains(JobPriority defaultPriority, bool defaultUseSyncFence, JobsTestFlags flags = kJobsTestDefaultFlags)
	{
		// Create several long chains of dependencies using either job system or SyncFence()
		DependentJob jobs[kSimultaneousChains][kJobsPerChain];
		int inValues[kSimultaneousChains][kJobsPerChain] = {};
		int outValues[kSimultaneousChains][kJobsPerChain] = {};
		JobFence sortFence;

		// The first job is deliberately slow so we have to wait on the result
		// kSortCount can be adjusted (e.g for different platforms) without breaking the test
		BubbleSortJob sortJob;
		const int kSortCount = 1000;
		int sortValues[kSortCount];
		int sortOutput = -1;
		sortJob.count = kSortCount;
		sortJob.input = sortValues;
		sortJob.output = &sortOutput;
		sortJob.started = false;
		for (int i = 0; i < kSortCount; ++i)
		{
			// "Random" values in range 3-1000
			sortValues[i] = 3 + ((i + 1) * 347) % 997;
		}
		sortValues[kSortCount-1] = 3;
		ScheduleJob(sortFence, BubbleSortFindSmallest, &sortJob);

		while (!sortJob.started) {}

		JobFence lastFence;
		JobFence chainFences[kSimultaneousChains];
		for (int c = 0; c < kSimultaneousChains; ++c)
		{
			for (int j = 0; j < kJobsPerChain; ++j)
			{
				JobPriority priority = defaultPriority;
				if (flags & kJobsTestMixPriorities)
					priority = (c & 1) ? kHighJobPriority : kNormalJobPriority;

				bool useSyncFence = defaultUseSyncFence;
				if (flags & kJobsTestMixSyncFenceAndDepends)
					useSyncFence = (c & 2) != 0;

				inValues[c][j] = j * 5 + 1;
				DependentJob& job = jobs[c][j];
				JobFence dependsOn = (j > 0) ? lastFence : sortFence;
				ClearFenceWithoutSync(lastFence);
				if (useSyncFence)
					job.dependencies[0] = dependsOn;
				job.inputs[0] = (j > 0) ? &outValues[c][j-1] : &sortOutput;
				job.inputs[1] = &inValues[c][j];
				job.output = &outValues[c][j];
				job.started = false;

				if (useSyncFence)
					ScheduleJob(lastFence, DependentAdd, &job, priority);
				else
					ScheduleJobDepends(lastFence, DependentAdd, &job, dependsOn, priority);

				ClearFenceWithoutSync(dependsOn);
			}
			chainFences[c] = lastFence;
			ClearFenceWithoutSync(lastFence);
		}
		ClearFenceWithoutSync(sortFence);

		int expected = 3 + ((kJobsPerChain * (kJobsPerChain - 1)) / 2) * 5 + kJobsPerChain;
		for (int c = kSimultaneousChains-1; c >= 0; --c)
		{
			SyncFence(chainFences[c]);
			int result = outValues[c][kJobsPerChain-1];
			CHECK_EQUAL(expected, result);
		}
	}
}

SUITE(JobsTests)
{
	TEST (Jobs_LongDependencyChains_Depends_NormalPriority)
	{
		TestLongDependencyChains<8, 16>(kNormalJobPriority, false);
	}

	TEST (Jobs_LongDependencyChains_SyncFence_NormalPriority)
	{
		TestLongDependencyChains<8, 16>(kNormalJobPriority, true);
	}

	TEST (Jobs_LongDependencyChains_MixedSyncFenceAndDepends_NormalPriority)
	{
		TestLongDependencyChains<8, 16>(kNormalJobPriority, false, kJobsTestMixSyncFenceAndDepends);
	}

	TEST (Jobs_LongDependencyChains_Depends_HighPriority)
	{
		TestLongDependencyChains<8, 16>(kHighJobPriority, false);
	}

#if 0
	TEST (Jobs_LongDependencyChains_MixedSyncFenceAndDepends_MixedPriorities)
	{
		TestLongDependencyChains<8, 16>(kNormalJobPriority, false, kJobsTestMixSyncFenceAndDepends | kJobsTestMixPriorities);
	}

	TEST (Jobs_LongDependencyChains_Depends_MixedPriorities)
	{
		TestLongDependencyChains<8, 16>(kNormalJobPriority, false, kJobsTestMixPriorities);
	}

	TEST (Jobs_LongDependencyChains_SyncFence_MixedPriorities)
	{
		TestLongDependencyChains<8, 16>(kNormalJobPriority, true, kJobsTestMixPriorities);
	}

	TEST (Jobs_LongDependencyChains_SyncFence_HighPriority)
	{
		TestLongDependencyChains<8, 16>(kHighJobPriority, true);
	}

	TEST (Jobs_LongDependencyChains_MixedSyncFenceAndDepends_HighPriority)
	{
		TestLongDependencyChains<8, 16>(kHighJobPriority, false, kJobsTestMixSyncFenceAndDepends);
	}
#endif
}

SUITE(ExtendedJobsTests)
{
	TEST (ExtendedJobs_LongDependencyChains_Depends_NormalPriority)
	{
		TestLongDependencyChains<50, 50>(kNormalJobPriority, false);
	}

	TEST (ExtendedJobs_LongDependencyChains_SyncFence_NormalPriority)
	{
		TestLongDependencyChains<50, 50>(kNormalJobPriority, true);
	}

	TEST (ExtendedJobs_LongDependencyChains_MixedSyncFenceAndDepends_NormalPriority)
	{
		TestLongDependencyChains<50, 50>(kNormalJobPriority, false, kJobsTestMixSyncFenceAndDepends);
	}


#if 0

	TEST (ExtendedJobs_LongDependencyChains_Depends_MixedPriorities)
	{
		TestLongDependencyChains<50, 50>(kNormalJobPriority, false, kJobsTestMixPriorities);
	}

	TEST (ExtendedJobs_LongDependencyChains_SyncFence_MixedPriorities)
	{
		TestLongDependencyChains<50, 50>(kNormalJobPriority, true, kJobsTestMixPriorities);
	}

	TEST (ExtendedJobs_LongDependencyChains_MixedSyncFenceAndDepends_MixedPriorities)
	{
		TestLongDependencyChains<50, 50>(kNormalJobPriority, false, kJobsTestMixSyncFenceAndDepends | kJobsTestMixPriorities);
	}

	TEST (ExtendedJobs_LongDependencyChains_Depends_HighPriority)
	{
		TestLongDependencyChains<50, 50>(kHighJobPriority, false);
	}

	TEST (ExtendedJobs_LongDependencyChains_SyncFence_HighPriority)
	{
		TestLongDependencyChains<50, 50>(kHighJobPriority, true);
	}

	TEST (ExtendedJobs_LongDependencyChains_MixedSyncFenceAndDepends_HighPriority)
	{
		TestLongDependencyChains<50, 50>(kHighJobPriority, false, kJobsTestMixSyncFenceAndDepends);
	}
#endif

}

#endif
