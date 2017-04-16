#include "PluginPrefix.h"
#include "AllocatorLabels.h"
#include "Allocator/MemoryMacros.h"
#include "Allocator/MemoryManager.h"
#include "Allocator/AllocationHeader.h"

const char* const MemLabelName[] =
{
	"TempLabels",
#define DO_LABEL(Name)
#define DO_TEMP_LABEL(Name) #Name ,
#include "AllocatorLabelNames.h"
#undef DO_TEMP_LABEL
#undef DO_LABEL
	"RegularLabels",
#define DO_LABEL(Name) #Name ,
#define DO_TEMP_LABEL(Name)
#include "AllocatorLabelNames.h"
#undef DO_TEMP_LABEL
#undef DO_LABEL
};

