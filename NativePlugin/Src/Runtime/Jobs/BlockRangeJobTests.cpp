#include "UnityPrefix.h"
#include "BlockRangeJob.h"
#include "Runtime/Jobs/JobSystem.h"
#include "Runtime/Testing/Testing.h"
#include "Runtime/Testing/TestFixtures.h"
#include "Runtime/Utilities/ArrayUtility.h"

#if ENABLE_UNIT_TESTS

SUITE (BlockRangeJobTests_BalancedWorkLoad)
{
	class BlockRangeBalancedWorkloadFixture : public TestFixtureBase
	{
	private:
		static const size_t mixedDataWorkload[];
		static const size_t mixedDataGroupCount;
		
	public:
		BlockRange tasks[kMaximumBlockRangeCount];
		dynamic_array<BlockRange> subTasks;
		dynamic_array<unsigned int> subTaskGroupIndices;

		const size_t* getMixedData() const { return mixedDataWorkload; }
		const size_t getMixedDataGroupCount() const { return mixedDataGroupCount; }
		const size_t getMixedDataTotalWork() const {
			size_t totalSize = 0;
			for(size_t i = 0; i < mixedDataGroupCount; ++i)
				totalSize += mixedDataWorkload[i];
			return totalSize;
		}
		
		void CheckBalancedWorkload(float tolerance, int numWorkerThreads)
		{
			
			// Arrange: set up a workload
			const int totalTasks = CalculateJobCountWithMinIndicesPerJob(getMixedDataTotalWork(), 1, numWorkerThreads);
			const size_t sizePerTask = std::ceil(getMixedDataTotalWork() / (float)totalTasks);
			
			BlockRangeBalancedWorkload workload(tasks, sizePerTask);
			
			// Act: add the groups to the workload
			const size_t* groupSizes = getMixedData();
			for(size_t i = 0; i < getMixedDataGroupCount(); ++i)
				AddGroupToWorkload(workload, groupSizes[i], subTasks, subTaskGroupIndices);
			
			// Assert: Check that all groups are within tolerance of the first group
			size_t firstTaskSize = 0;
			for(int i = 0; i < tasks[0].rangeSize; ++i)
				firstTaskSize += subTasks[tasks[0].startIndex + i].rangeSize;
			
			size_t sizeTolerance = tolerance * firstTaskSize;
			
			for(int task = 1; task < totalTasks; ++task)
			{
				size_t taskSize = 0;
				for(int i = 0; i < tasks[task].rangeSize; ++i)
					taskSize += subTasks[tasks[task].startIndex + i].rangeSize;
				
				CHECK_CLOSE(firstTaskSize, taskSize, sizeTolerance);
			}
		}
	};
	
	const size_t BlockRangeBalancedWorkloadFixture::mixedDataWorkload[] = { 123, 456, 789, 1234 };
	const size_t BlockRangeBalancedWorkloadFixture::mixedDataGroupCount = ARRAY_SIZE(mixedDataWorkload);
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, EmptyGroup_GeneratesNoSubtasks)
	{
		// Arrange: set up a workload
		BlockRangeBalancedWorkload workload(tasks, 10);
		AddGroupToWorkload(workload, 15, subTasks, subTaskGroupIndices);
		
		size_t originalSubtaskCount = subTasks.size();
		
		// Act: add an empty group to the workload
		BlockRange emptyGroupRange = AddGroupToWorkload(workload, 0, subTasks, subTaskGroupIndices);
		
		// Assert: Check that the empty group generated a zero-sized task range and no subtask entries
		CHECK_EQUAL(0, emptyGroupRange.rangeSize);
		CHECK_EQUAL(originalSubtaskCount, subTasks.size());
		CHECK_EQUAL(subTasks.size(), subTaskGroupIndices.size());
	}
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, EmptyGroup_ConsumesGroupIndex)
	{
		// Arrange: set up a workload
		BlockRangeBalancedWorkload workload(tasks, 10);
		
		// Act: add an empty group to the workload, then a non-empty group
		AddGroupToWorkload(workload, 0, subTasks, subTaskGroupIndices);
		AddGroupToWorkload(workload, 1, subTasks, subTaskGroupIndices);
		
		// Assert: Check that the empty group occupied index 0, so the non-empty group has index 1
		CHECK_EQUAL(1, subTaskGroupIndices.size());
		CHECK_EQUAL(1, subTaskGroupIndices.back());
	}
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, SmallGroups_CombineToOneTask)
	{
		// Arrange: set up a workload
		BlockRangeBalancedWorkload workload(tasks, 10);
		
		// Act: add small groups to the workload that should be getting combined into one task
		for(int i = 0; i < 5; ++i)
			AddGroupToWorkload(workload, 1, subTasks, subTaskGroupIndices);
		AddGroupToWorkload(workload, 10, subTasks, subTaskGroupIndices);
		
		// Assert: Check that task 0 covers all the small groups, plus part of the big group
		CHECK_EQUAL(0, tasks[0].startIndex);
		CHECK_EQUAL(6, tasks[0].rangeSize);
	}
	
	// Check that work-balancing across tasks is working correctly on a variety of multi-core setups
	// Explicitly test particular common core counts, so that we're not relying on the test farm itself
	// having sufficient diversity, and avoiding flip-flops from the tests running on different agents
	// that have different core counts.
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, MixedGroups_IsBalancedAcrossTasks_DualCore)
	{
		CheckBalancedWorkload(0.05f, 2);
	}
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, MixedGroups_IsBalancedAcrossTasks_QuadCore)
	{
		CheckBalancedWorkload(0.05f, 4);
	}
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, MixedGroups_IsBalancedAcrossTasks_HexCore)
	{
		CheckBalancedWorkload(0.05f, 6);
	}
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, MixedGroups_IsBalancedAcrossTasks_OctoCore)
	{
		CheckBalancedWorkload(0.05f, 8);
	}
	
	// Do one more check using the actual number of worker threads in the current machine. This will almost
	// certainly be duplicating one of the previous four tests, but will help us catch any hardware which has an
	// unusual number of cores. (This might flip-flop, but it's better than not catching the failure at all,
	// and the presence of the previous four tests should make it pretty obvious what's going on if it fails).
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, MixedGroups_IsBalancedAcrossTasks_DynamicCores)
	{
		CheckBalancedWorkload(0.05f, GetJobQueueThreadCount());
	}
	
	TEST_FIXTURE (BlockRangeBalancedWorkloadFixture, MixedGroups_AllWorkAccountedFor)
	{
		// Arrange: set up a workload
		const int totalTasks = CalculateJobCountWithMinIndicesPerJob(getMixedDataTotalWork(), 10);
		const size_t sizePerTask = std::ceil(getMixedDataTotalWork() / (float)totalTasks);
		
		BlockRangeBalancedWorkload workload(tasks, sizePerTask);
		
		// Act: add the groups to the workload
		const size_t* groupSizes = getMixedData();
		for(size_t i = 0; i < getMixedDataGroupCount(); ++i)
			AddGroupToWorkload(workload, groupSizes[i], subTasks, subTaskGroupIndices);
		
		// Assert: check that every work item is in a subtask touched by a task, and only once
		std::vector<dynamic_array<bool> > work;
		for(int i = 0; i < getMixedDataGroupCount(); ++i)
		{
			work.push_back(dynamic_array<bool>(groupSizes[i], false, kMemTempJobAlloc));
		}
		
		size_t totalWorkDone = 0;
		for(int taskIndex = 0; taskIndex < totalTasks; ++taskIndex)
		{
			BlockRange& task = tasks[taskIndex];
			for(int subTaskIndex = 0; subTaskIndex < task.rangeSize; ++subTaskIndex)
			{
				BlockRange& subTask = subTasks[task.startIndex + subTaskIndex];
				dynamic_array<bool>& groupWork = work[subTaskGroupIndices[task.startIndex + subTaskIndex]];
				
				for(int workIndex = 0; workIndex < subTask.rangeSize; ++workIndex)
				{
					// Check that the work has not been done before
					CHECK_EQUAL(false, groupWork[subTask.startIndex + workIndex]);
					groupWork[subTask.startIndex + workIndex] = true;
					++totalWorkDone;
				}
			}
		}
		// Check that all work was done
		CHECK_EQUAL(getMixedDataTotalWork(), totalWorkDone);
	}
	
	TEST (Basic)
	{
		//	This is really more of a Stress Test than Unit Test as we have a dependency on GetJobQueueThreadCount()
		//	which is used to clamp the maximum number of jobs we can schedule.
		//	Should catch breakages to ConfigureBlockRangesWithMinIndicesPerJob
		static const int kMaxIndices = 100;
		static const int kUpperMinIndices = 100;	// MinIndices becomes inconveniently named here...:-)
		
		for (int numIndices=1; numIndices < kMaxIndices; numIndices++)
		{
			for (int minNumIndices=1; minNumIndices < kUpperMinIndices; minNumIndices++)
			{
				BlockRange	blocks[kMaximumBlockRangeCount];
				
				// Work out how many jobs this combo willl require and configure the block ranges
				const int jobCount = ConfigureBlockRangesWithMinIndicesPerJob (blocks, numIndices, minNumIndices);
				
				// Check we haven't tried to make too many jobs
				const	bool	acceptableNumberOfJobs = (jobCount<=kMaximumBlockRangeCount);
				CHECK_EQUAL (true, acceptableNumberOfJobs);
				
				// And that there is at least one
				const	bool	atLeastOneJob = (jobCount >= 1);
				CHECK_EQUAL (true, atLeastOneJob);
				
				int	total = 0;
				int	precedingHighest = blocks[0].rangeSize;
				for (int i=0; i < jobCount; i++)
				{
					// Ignore final
					if (i < (jobCount-1))
					{
						if (blocks[i].rangeSize > precedingHighest)
						{
							precedingHighest = blocks[i].rangeSize;
						}
					}
					const bool hasWork = (blocks[i].rangeSize!=0);
					CHECK_EQUAL (hasWork, true);
					
					// Check no job has too much work
					const int maxPayload = (numIndices+(jobCount-1)) / jobCount;
					const bool	rightSizedJob = (blocks[i].rangeSize <= maxPayload);
					CHECK_EQUAL (rightSizedJob, true);
					
					// Check job count is reasonable
					const bool	correctNumberOfJobs = (blocks[i].rangesTotal == jobCount);
					CHECK_EQUAL (correctNumberOfJobs, true);
					
					// Check job will work start the domain
					const bool	indexGood = (blocks[i].startIndex <= numIndices);
					CHECK_EQUAL (indexGood, true);
					
					// Check job will finish within the domain
					const bool	totalReasonable = ((blocks[i].startIndex + blocks[i].rangeSize) <= numIndices);
					CHECK_EQUAL (totalReasonable, true);
					
					// Keep a total count of all the work across jobs
					total += blocks[i].rangeSize;
				}
				
				// Check that the total volume of work is as it should be
				const bool totalVolumeCorrect = (total==numIndices);
				CHECK_EQUAL (totalVolumeCorrect, true);
				
				// Check that final job has no more work than any of the preceding (i.e. is a genuine quotient/divisor remainder)
				const bool	remainderAppropriatelySized = (blocks[jobCount-1].rangeSize <= precedingHighest);
				CHECK_EQUAL (remainderAppropriatelySized, true);
			}
		}
	}
}


SUITE (BlockRangeJobTests_CombineRanges)
{
	TEST (CombineBlockRangesOrdered)
	{
		int test[] = { 0, 1, 2, 3, 4, 5, 6 };

		BlockRange range[4];

		range[0].startIndex = 0;
		range[0].rangeSize = 1;

		range[1].startIndex = 3;
		range[1].rangeSize = 1;

		range[2].startIndex = 4;
		range[2].rangeSize = 2;

		range[3].startIndex = 6;
		range[3].rangeSize = 0;

		CHECK_EQUAL(4, CombineBlockRangesOrdered(test, range, 4));
		CHECK_EQUAL(0, test[0]);
		CHECK_EQUAL(3, test[1]);
		CHECK_EQUAL(4, test[2]);
		CHECK_EQUAL(5, test[3]);
	}

	TEST (CombineBlockRangesUnordered)
	{
		int test[] = { 0, 1, 2, 3, 4, 5, 6 };
		
		BlockRange range[4];
		
		range[0].startIndex = 0;
		range[0].rangeSize = 1;
		
		range[1].startIndex = 3;
		range[1].rangeSize = 1;
		
		range[2].startIndex = 4;
		range[2].rangeSize = 2;
		
		range[3].startIndex = 6;
		range[3].rangeSize = 0;
		
		CHECK_EQUAL(4, CombineBlockRangesUnordered(test, range, 4));
		CHECK_EQUAL(0, test[0]);
		CHECK_EQUAL(5, test[1]);
		CHECK_EQUAL(4, test[2]);
		CHECK_EQUAL(3, test[3]);
	}

}


#endif
