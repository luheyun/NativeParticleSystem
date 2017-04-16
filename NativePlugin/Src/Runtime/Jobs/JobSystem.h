#pragma once

// these functions are for management of the job system itself, and are special use. if you are wanting to use the job system
// then you should #include jobs.h instead.

void CreateJobSystem();
void DestroyJobSystem();

int GetJobQueueThreadCount();

// Pops one job and executes it.
// First looks at the high priority stack, then at the job queue to pick a job that can be executed.
// Returns true if a job was executed. Returns false if there were no jobs in the queue.
// For example you could use it to execute some work on the main thread while waiting for something.
// (If you are waiting on a specific job you should use SyncFence instead)
bool ExecuteOneJobQueueJob();
