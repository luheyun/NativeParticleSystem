#pragma once

class JobGroup;

struct JobGroupID
{
	JobGroup*   info;
	unsigned    version;

	JobGroupID ()
		: info(0)
		, version(0u)
	{}
};
