#include "UnityPrefix.h"
#include "LowLevelDefaultAllocator.h"
#include "Runtime/Allocator/MemoryManager.h"

#if ENABLE_MEMORY_MANAGER


void* LowLevelAllocator::Malloc(size_t size) { return MemoryManager::LowLevelAllocate(size); }
void* LowLevelAllocator::Realloc(void* ptr, size_t size, size_t oldSize) { return MemoryManager::LowLevelReallocate(ptr, size, oldSize); }
void  LowLevelAllocator::Free(void* ptr, size_t oldSize) { MemoryManager::LowLevelFree(ptr, oldSize); }

#endif
