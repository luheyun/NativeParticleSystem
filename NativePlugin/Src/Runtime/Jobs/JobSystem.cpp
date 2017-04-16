#include "PluginPrefix.h"
#include "JobSystem.h"
#include "Internal/JobQueue.h"

void CreateJobSystem()
{
#	if !UNITY_EXTERNAL_TOOL
	CreateJobQueue("Unity Job System", "Worker Thread");
#	endif
}

void DestroyJobSystem()
{
#	if !UNITY_EXTERNAL_TOOL
	DestroyJobQueue();
#	endif
}

int GetJobQueueThreadCount()
{
	return GetJobQueue().GetThreadCount();
}

bool ExecuteOneJobQueueJob()
{
	return JobQueueCreated() ? GetJobQueue().ExecuteOneJob() : false;
}
