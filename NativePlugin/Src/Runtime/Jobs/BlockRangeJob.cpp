#include "PluginPrefix.h"
#include "BlockRangeJob.h"
#include "Jobs.h"
#include "JobSystem.h"

int CalculateJobCountWithMinIndicesPerJob (int totalIndices, int minimumIndicesPerJob)
{
	return CalculateJobCountWithMinIndicesPerJob(totalIndices, minimumIndicesPerJob, GetJobQueueThreadCount());
}

int CalculateJobCountWithMinIndicesPerJob (int totalIndices, int minimumIndicesPerJob, int workerThreads)
{
	//Assert(totalIndices != 0);
	//Assert(minimumIndicesPerJob != 0);

	// If there are no worker threads then it will run on the main thread
	// thus there is no reason to split the work into multiple jobs
	if (workerThreads == 0)
		return 1;

	// We want to prevent tiny jobs.
	// (eg. for culling minimumIndicesPerJob means we have for example 32 bounding volumes to be culled per job)
	int jobCountBasedOnMinimumSize = (totalIndices + (minimumIndicesPerJob-1)) / minimumIndicesPerJob;

	// Most jobs are waited for on the thread the schedules them. So to even it out (workerThreads + 1).
	// Then * 2 because often jobs are not executed perfectly even so if some jobs are longer at least it distributes a bit more.
	// This is no perfect math, just empirically tweaked on profiling data.
	int jobCountBasedOnWorkerThreads = (workerThreads+1) * 2;

	int jobCount = std::min<int> (jobCountBasedOnMinimumSize, jobCountBasedOnWorkerThreads);
	jobCount = std::min<int> (jobCount, kMaximumBlockRangeCount);

	return jobCount;
}

int ConfigureBlockRangesWithMinIndicesPerJob (BlockRange* blockRanges, int totalIndices, int minimumIndicesPerJob)
{
	int jobCount = CalculateJobCountWithMinIndicesPerJob (totalIndices, minimumIndicesPerJob);
	//	Configure the index ranges for each job and ensure jobs are balanced in work load
	//	(note ConfigureBlockRanges can reduce the notional initial jobCount to ensure last 'remainder' job isn't over-indexing)
	jobCount = ConfigureBlockRanges (blockRanges, totalIndices, jobCount);
	return jobCount;
}

int ConfigureBlockRanges (BlockRange* blockRanges, int totalIndices, int blockRangeCount)
{
	//	Ensure remainder in final job is equal to or less than preceding jobs
	size_t indicesPerJob = (totalIndices + (blockRangeCount-1)) / blockRangeCount;
	//	And adjust job count, if required, following remainder management
	size_t revisedNumJobs = (totalIndices + (indicesPerJob-1)) / indicesPerJob;

	for (size_t i=0;i<revisedNumJobs;i++)
	{
		size_t startIndex = indicesPerJob * i;
		blockRanges[i].startIndex = startIndex;

		// Normal jobs use indicesPerJob
		if (i != revisedNumJobs-1)
		{
			blockRanges[i].rangeSize = indicesPerJob;
		}
		// Last job uses the rest
		else
		{
			size_t rangeSize = totalIndices - startIndex;
			blockRanges[i].rangeSize = rangeSize;
			//	Ensure final job picks up a true remainder to avoid over-indexing
			//	Final job is probably/usually last to start executing (when in a queue) so this matters
			//Assert (rangeSize <= indicesPerJob);
		}
		blockRanges[i].rangesTotal = revisedNumJobs;
	}

	return revisedNumJobs;
}

BlockRange AddGroupToWorkload(BlockRangeBalancedWorkload& workload, size_t groupSize, dynamic_array<BlockRange>& subTasks, dynamic_array<unsigned int>& subTaskGroupIndices)
{
	BlockRange result = BlockRange(subTasks.size(), 0);
	size_t groupOffset = 0;
	while (groupOffset < groupSize)
	{
		if (workload.currentTaskWork >= workload.workPerTask)
		{
			workload.tasks[++workload.currentTaskIndex] = BlockRange(subTasks.size(), 0);
			workload.currentTaskWork = 0;
		}
		
		BlockRange& currentTask = workload.tasks[workload.currentTaskIndex];
		
		const size_t subTaskSize = std::min(groupSize - groupOffset, workload.workPerTask - workload.currentTaskWork);
		
		subTasks.push_back(BlockRange(groupOffset, subTaskSize));
		subTaskGroupIndices.push_back(workload.groupCount);
		++currentTask.rangeSize;
		
		groupOffset += subTaskSize;
		workload.currentTaskWork += subTaskSize;
		
		//Assert(groupOffset <= groupSize);
		//Assert(workload.currentTaskWork <= workload.workPerTask);
	}
	
	++workload.groupCount;
	result.rangeSize = subTasks.size() - result.startIndex;
	return result;
}

namespace BlockRangeInternal
{
	UInt32 GetSizeFromLastBlockRange (BlockRange* blocks, UInt32 nbBlocks)
	{
		for (int i = nbBlocks-1;i >= 0;i--)
		{
			if (blocks[i].rangeSize != 0)
				return blocks[i].rangeSize + blocks[i].startIndex;
		}

		return 0;
	}

	int PopLastNodeIndex (BlockRange* blocks, UInt32 nbBlocks, int currentBlock)
	{
		for (int i = nbBlocks-1;i > currentBlock;i--)
		{
			if (blocks[i].rangeSize != 0)
		{
				blocks[i].rangeSize--;
				return blocks[i].rangeSize + blocks[i].startIndex;
			}			
		}

		return -1;
	}
}

