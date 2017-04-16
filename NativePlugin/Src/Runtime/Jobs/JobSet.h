#pragma once

#include "Jobs.h"

/////*****  Unless you exactly know you want this. DO NOT USE THIS! See Jobs.h
/// Creates a JobFence that represents multiple jobs that need to be completed when SyncFence is called.
/// jobCount must match the amount of jobs you schedule into the jobSet exactly.

/// JobSet scheduling can be used if you want to run several independent "foreach" jobs (which can be executed in parallel)
/// and use single fence for the synchronization.
/// This means that the logic is similar to ScheduleDifferentJobsConcurrentDepends and ScheduleDifferentJobsConcurrentDepends
/// but applies to "foreach" jobs scheduling.

/// Before using this, please see Jobs.h, JobSet is only used for some very specialized cases.
/// The functions in Jobs.h are to be used most commonly.


// ----  CODE EXAMPLE ----
// JobFence jobSet;
// BeginJobSet (jobSet, 2);
// ScheduleJobForEachJobSet (jobSet, ...);
// ScheduleJobForEachJobSet (jobSet, ...);
// EndJobSet (jobSet);
// ...
// SyncFence (jobSet); // The job system will wait for both ScheduleJobForEachJobSet to complete.

/// NOTE:
/// * You are not allowed to use SyncFence or non-JobSet scheduling functions on the fence between calling BeginJobSet and EndJobSet.
///   You may only use the ScheduleJobForEachJobSet on the fence between those two calls.
/// * The amount of jobs you schedule has to match the jobCount passed into BeginJobSet exactly.

void BeginJobSet (JobFence& jobSet, int jobCount);

template<typename T>
inline void ScheduleJobForEachJobSet(JobFence& jobSet, void concurrentJobFunc(T*, unsigned), T* jobData, int iterationCount, const JobFence& dependsOn, void combineJobFunc(T*), JobPriority priority = kNormalJobPriority);
template<typename T>
inline void ScheduleJobForEachJobSet(JobFence& jobSet, void concurrentJobFunc(T*, unsigned), T* jobData, int iterationCount, const JobFence& dependsOn, JobPriority priority = kNormalJobPriority);

void EndJobSet (JobFence& jobSet, JobPriority priority = kNormalJobPriority);


// INLINES

template<typename T>
inline void ScheduleJobForEachJobSet(JobFence& jobSet, void concurrentJobFunc(T*, unsigned), T* jobData, int iterationCount, const JobFence& dependsOn, void combineJobFunc(T*), JobPriority priority)
{
	void ScheduleJobForEachJobSetInternal(JobFence& jobSet, JobForEachFunc concurrentJobFunc, void* userData, int iterationCount, const JobFence& dependsOn, JobFunc combineJobFunc, JobPriority priority);
	ScheduleJobForEachJobSetInternal(jobSet, reinterpret_cast<JobForEachFunc*>(concurrentJobFunc), jobData, iterationCount, dependsOn, reinterpret_cast<JobFunc*>(combineJobFunc), priority);
}

template<typename T>
inline void ScheduleJobForEachJobSet(JobFence& jobSet, void concurrentJobFunc(T*, unsigned), T* jobData, int iterationCount, const JobFence& dependsOn, JobPriority priority)
{
	ScheduleJobForEachJobSet(jobSet, concurrentJobFunc, jobData, iterationCount, dependsOn, reinterpret_cast<JobFunc*>(NULL), priority);
}
