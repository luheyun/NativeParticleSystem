#include "UnityPrefix.h"

#if ENABLE_UNIT_TESTS

#include "Configuration/UnityConfigure.h"
#include "Runtime/Testing/Testing.h"
#include "Runtime/Testing/TestFixtures.h"
#include "Runtime/Jobs/Jobs.h"
#include "Runtime/Jobs/JobBatchDispatcher.h"

#define RUN_OVER_NIGHT 0

SUITE (JobQueuePerformanceTests)
{
	struct TestData
	{
		JobFence 	dependency;
		int  		expectedValue;
		int* 		sharedTestData;
		int 		parallelCount;
		~TestData()
		{
			ClearFenceWithoutSync(dependency);
		}
	};

	static void IncrementAndExpectData (TestData* data)
	{
		SyncFence(data->dependency);

		CHECK_EQUAL(data->expectedValue, *data->sharedTestData);
		(*data->sharedTestData)++;
	}

	static void IncrementAndExpectDataForEach (TestData* data, unsigned index)
	{
		SyncFence(data[0].dependency);

		int offset = index * data[0].parallelCount;

		CHECK_EQUAL(data[offset].expectedValue, *data[offset].sharedTestData);
		(*data[offset].sharedTestData)++;
	}

	enum Mode { kDirectCall, kSyncFence, kSyncFenceDispatcher, kDependency, kDependencyDispatcher };

	template<int kParallelCount, int kChainCount>
	static void RunTests(Mode mode, bool useForEach)
	{
		TestData dataAll[kParallelCount][kChainCount];
		int sharedValueAll[kParallelCount];
		JobFence previousFenceAll[kParallelCount];

		// schedule
		for (int p = 0; p < kParallelCount; ++p)
		{
			TestData* data = dataAll[p];
			int& sharedValue = sharedValueAll[p];

			sharedValue = 0;

			for (int i = 0; i < kChainCount; ++i)
			{
				data[i].sharedTestData = &sharedValue;
				data[i].expectedValue = i;
				data[i].parallelCount = kChainCount;
			}
		}

		// Foreach
		if (useForEach)
		{
			JobFence& previousFence = previousFenceAll[0];
			
			JobBatchDispatcher dispatch;
			for (int i = 0; i < kChainCount; ++i)
			{					
				JobFence fence;
				if (mode == kDirectCall)
				{
					for (int k = 0; k < kParallelCount; ++k)
						IncrementAndExpectDataForEach(dataAll[0] + i, k);
				}
				else if (mode == kSyncFence)
				{
					AssertString("not supported");
				}
				else if (mode == kSyncFenceDispatcher)
				{
					AssertString("not supported");
				}
				else if (mode == kDependency)
				{
					ScheduleJobForEachDepends(fence, IncrementAndExpectDataForEach, dataAll[0] + i, kParallelCount, previousFence);
				}
				else if (mode == kDependencyDispatcher)
				{
					dispatch.ScheduleJobForEachDepends(fence, IncrementAndExpectDataForEach, dataAll[0] + i, kParallelCount, previousFence);
				}

				ClearFenceWithoutSync(previousFence);
				previousFence = fence;
				ClearFenceWithoutSync(fence);
			}
		}
		// Individual jobs
		else
		{
			JobBatchDispatcher dispatch;

			for (int p = 0; p < kParallelCount; ++p)
			{
				TestData* data = dataAll[p];
				JobFence& previousFence = previousFenceAll[p];

				for (int i = 0; i < kChainCount; ++i)
				{
					JobFence fence;
					if (mode == kDirectCall)
					{
						IncrementAndExpectData(&data[i]);
					}
					else if (mode == kSyncFence)
					{
						data[i].dependency = previousFence;
						ScheduleJob(fence, IncrementAndExpectData, &data[i]);
					}
					else if (mode == kSyncFenceDispatcher)
					{
						data[i].dependency = previousFence;
						dispatch.ScheduleJob(fence, IncrementAndExpectData, &data[i]);
					}
					else if (mode == kDependency)
					{
						ScheduleJobDepends(fence, IncrementAndExpectData, &data[i], previousFence);
					}
					else if (mode == kDependencyDispatcher)
					{
						dispatch.ScheduleJobDepends(fence, IncrementAndExpectData, &data[i], previousFence);
					}

					ClearFenceWithoutSync(previousFence);
					previousFence = fence;
					ClearFenceWithoutSync(fence);
				}
			}	
		}

		// complete jobs
		for (int p = 0; p < kParallelCount; ++p)
		{
			SyncFence(previousFenceAll[p]);

			CHECK_EQUAL(kChainCount, sharedValueAll[p]);
		}
	}

#if RUN_OVER_NIGHT
	// Takes ~40mins
	//const int kParallelCount = 200;
	//const int kChainCount = 10;
	//const int kRunNTimes = 50000;
	// Takes ~40mins
	//const int kParallelCount = 40;
	//const int kChainCount = 6;
	//const int kRunNTimes = 500000;
	// Takes ~340mins
	const int kParallelCount = 200;
	const int kChainCount = 10;
	const int kRunNTimes = 500000;
#else
	const int kParallelCount = 40;
	const int kChainCount = 6;
	const int kRunNTimes = 1;
#endif
	
	TEST (PreallocateJobGroups)
	{
		RunTests<kParallelCount, kChainCount>(kDependency, false);
		RunTests<kParallelCount, kChainCount>(kDependency, true);
	}

	TEST (DirectCallMainThread)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kDirectCall, false);
	}
	
	TEST (JobChaining)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kDependency, false);
	}

	TEST (JobChaining_Dispatcher)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kDependencyDispatcher, false);
	}

	TEST (JobSyncFence)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kSyncFence, false);
	}

	TEST (JobSyncFence_Dispatcher)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kSyncFenceDispatcher, false);
	}
	
	TEST (DirectCallMainThread_ForEach)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kDirectCall, true);
	}

	TEST (JobChaining_ForEach)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kDependency, true);
	}

	TEST (JobChaining_ForEach_Dispatcher)
	{
		for (int i = 0; i < kRunNTimes; ++i)
			RunTests<kParallelCount, kChainCount>(kDependencyDispatcher, true);
	}
}

#endif
