#pragma once

#include "Internal/JobGroupID.h"

// JobFences are used to ensure a job has complete before any data is accessed.
// See SyncFence function
struct JobFence
{
	JobFence ()
	{
	}

	void operator = (const JobFence& fence)
	{
		group = fence.group;
	}

	void operator = (const JobGroupID& inGroup)
	{
		group = inGroup;
	}

	~JobFence ()
	{
	}
	
	JobGroupID group;
};
