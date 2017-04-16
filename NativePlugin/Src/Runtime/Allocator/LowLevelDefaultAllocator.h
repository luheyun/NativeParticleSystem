#ifndef LOW_LEVEL_DEFAULT_ALLOCATOR_H_
#define LOW_LEVEL_DEFAULT_ALLOCATOR_H_

#if ENABLE_MEMORY_MANAGER

class LowLevelAllocator
{
public:
	static void* Malloc(size_t size);
	static void* Realloc(void* ptr, size_t size, size_t oldSize);
	static void  Free(void* ptr, size_t oldSize);
};

#if UNITY_XENON
class LowLevelAllocatorDebugMem
{
public:
	static void* Malloc(size_t size);
	static void* Realloc(void* ptr, size_t size, size_t oldSize);
	static void  Free(void* ptr, size_t oldSize);
};
#endif

#endif
#endif // LOW_LEVEL_DEFAULT_ALLOCATOR_H_
