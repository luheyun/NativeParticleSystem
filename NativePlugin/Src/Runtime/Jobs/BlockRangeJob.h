#pragma once

#include "JobTypes.h"
#include "Runtime/Utilities/dynamic_array.h"

enum { kMaximumBlockRangeCount = 16 };

struct BlockRange
{
	// The index where we should start processing
	size_t                startIndex;
	
	// The amount of objects that should be processed by this job.
	// Sometimes jobs also write into this as an output,
	// which is then used by the combine job to combine the outputs.
	size_t                rangeSize;
	
	// The total amount of jobs scheduled
	size_t				  rangesTotal;
	
	BlockRange() { }
	BlockRange(size_t _startIndex, size_t _rangeSize) : startIndex(_startIndex), rangeSize(_rangeSize) { }
};

// When you make jobs that work on arrays.
// You usually use ScheduleJobForEach, but it is a good idea to split the work into fewer jobs than elements in the array.
// CalculateJobCountWithMinIndicesPerJob helps you choose the right amount of jobs.

// See Jobs.h for a code example.

int CalculateJobCountWithMinIndicesPerJob (int arrayLength, int minimumIndicesPerJob);
int CalculateJobCountWithMinIndicesPerJob (int arrayLength, int minimumIndicesPerJob, int workerThreads);
int ConfigureBlockRangesWithMinIndicesPerJob (BlockRange* blockRanges, int arrayLength, int minimumIndicesPerJob);
int ConfigureBlockRanges (BlockRange* blockRanges, int arrayLength, int blockRangeCount);

namespace BlockRangeInternal
{
	UInt32 GetSizeFromLastBlockRange (BlockRange* blocks, UInt32 nbBlocks);
	int PopLastNodeIndex (BlockRange* blocks, UInt32 nbBlocks, int currentBlock);
}

#if ENABLE_PROFILER
extern ProfilerInformation gProfilerCombineJob;
#endif

/// A common way to multithread the operations on arrays is to split the work into ranges.
/// For example in the culling system, we subdivide into ranges using ConfigureBlockRangesWithMinIndicesPerJob.
/// Each jobs outputs an IndexList of visible objects into it's range (We preallocate enough memory in the index array to contain all objects being visible)
/// The combine job is then responsible for combining the Index list ranges into index list.
/// Essentially clearing out the free space between the jobs.


// Gurantees that the output array are simply "packed" together without affecting the order.
template<class OutputT> inline
static size_t CombineBlockRangesOrdered (OutputT& inputOutputArray, const BlockRange* blocks, int nbBlocks)
{
	PROFILER_AUTO(gProfilerCombineJob, NULL);

	Assert(blocks[0].startIndex == 0);
	
	int visibleCountNodes = blocks[0].rangeSize;
	for (int i = 1; i < nbBlocks; ++i)
	{
		size_t endIndex = blocks[i].startIndex + blocks[i].rangeSize;
		for (size_t j = blocks[i].startIndex; j < endIndex; ++j)
			inputOutputArray[visibleCountNodes++] = inputOutputArray[j];
	}
	return visibleCountNodes;
}

// Does not gurantee anything about the order of the objects in the output.
template<class OutputT> inline
static size_t CombineBlockRangesUnordered (OutputT& inputOutputArray, const BlockRange* blocks, UInt32 nbBlocks)
{
	PROFILER_AUTO(gProfilerCombineJob, NULL);

	BlockRange tempBlocks[kMaximumBlockRangeCount];
	memcpy(tempBlocks, blocks, kMaximumBlockRangeCount * sizeof(BlockRange));

	Assert(tempBlocks[0].startIndex == 0);

	for (int i = 0; i < nbBlocks-1; ++i)
	{
		size_t endIndex = tempBlocks[i+1].startIndex;
		size_t actualEndIndex = tempBlocks[i].startIndex + tempBlocks[i].rangeSize;

		for (size_t j = actualEndIndex; j < endIndex; ++j)
		{
			int lastNodeIndex = BlockRangeInternal::PopLastNodeIndex(tempBlocks, nbBlocks, i);
			if ( lastNodeIndex == -1)
				break;
			inputOutputArray[j] = inputOutputArray[lastNodeIndex];
			tempBlocks[i].rangeSize++;
		}
	}

	return BlockRangeInternal::GetSizeFromLastBlockRange(tempBlocks, nbBlocks);
}

/// Another common multithreading problem is where the work you have is in groups (different sets of shared parameters), but the
/// size of the groups may be very different, so simply launching one job per group would be poorly balanced. The solution is to
/// divide the work within the groups into 'subtasks,' and then each job does multiple subtasks, often across multiple groups.
/// Use BlockRangeBalancedWorkload to calculate what the subtasks are, and which subtasks each top-level job should execute.

struct BlockRangeBalancedWorkload
{
	size_t currentTaskIndex;
	size_t currentTaskWork;
	size_t groupCount;
	
	BlockRange* tasks;
	size_t workPerTask;
	
	BlockRangeBalancedWorkload(BlockRange* _tasks, size_t _workPerTask)
	: currentTaskIndex(-1), currentTaskWork(-1), groupCount(0), tasks(_tasks), workPerTask(_workPerTask)
	{ }
};

/// Add one group of work to the workload. Returns the range of subtasks generated for this group, useful for combining afterwards.
BlockRange AddGroupToWorkload(BlockRangeBalancedWorkload& workload, size_t groupSize, dynamic_array<BlockRange>& subTasks, dynamic_array<unsigned int>& subTaskGroupIndices);
