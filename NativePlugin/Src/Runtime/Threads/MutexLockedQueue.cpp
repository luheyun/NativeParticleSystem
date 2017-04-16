#include "PluginPrefix.h"
#include "Runtime/Allocator/MemoryMacros.h"
#include "Runtime/Threads/MutexLockedQueue.h"

MutexLockedQueue* CreateMutexLockedQueue()
{
    return UNITY_NEW (MutexLockedQueue, kMemThread);
}

void DestroyMutexLockedQueue (MutexLockedQueue* queue)
{
    UNITY_DELETE (queue, kMemThread);
}