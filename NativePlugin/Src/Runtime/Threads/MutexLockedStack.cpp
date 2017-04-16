#include "PluginPrefix.h"
//#include "Runtime/Misc/AllocatorLabels.h"
#include "Runtime/Allocator/MemoryMacros.h"
#include "Runtime/Threads/MutexLockedStack.h"

MutexLockedStack* CreateMutexLockedStack ()
{
	return UNITY_NEW(MutexLockedStack, kMemThread);
}

void DestroyMutexLockedStack (MutexLockedStack* s)
{
	UNITY_DELETE(s, kMemThread);
}
