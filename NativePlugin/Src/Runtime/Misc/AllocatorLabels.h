#ifndef ALLOCATORLABELS_H
#define ALLOCATORLABELS_H

#include <stddef.h> // for NULL

enum MemLabelIdentifier
{
	kMemTempLabels,
	// add temp labels first in the enum
#define DO_LABEL(Name)
#define DO_TEMP_LABEL(Name) kMem##Name##Id ,
#include "AllocatorLabelNames.h"
#undef DO_TEMP_LABEL
#undef DO_LABEL
	kMemRegularLabels,
	// then add regular labels
#define DO_LABEL(Name) kMem##Name##Id ,
#define DO_TEMP_LABEL(Name)
#include "AllocatorLabelNames.h"
#undef DO_TEMP_LABEL
#undef DO_LABEL
	kMemLabelCount
};

struct AllocationRootReference;
struct ProfilerAllocationHeader;


EXPORT_COREMODULE typedef MemLabelIdentifier MemLabelId;
EXPORT_COREMODULE typedef MemLabelIdentifier MemLabelRef;

inline EXPORT_COREMODULE MemLabelId CreateMemLabel(MemLabelIdentifier id) { return id;}
inline EXPORT_COREMODULE MemLabelId CreateMemLabel(MemLabelIdentifier id, void* memoryOwner) { return id;}
inline EXPORT_COREMODULE MemLabelId SetCurrentMemoryOwner (MemLabelRef label) { return label; }
inline EXPORT_COREMODULE AllocationRootReference* GetRootReference (MemLabelId label) { return NULL; }
inline EXPORT_COREMODULE bool IsTempLabel (MemLabelId label) { return label < kMemRegularLabels; }
inline EXPORT_COREMODULE MemLabelIdentifier GetLabelIdentifier(MemLabelRef label) {return label;}


#define DO_LABEL_STRUCT(Name) const MemLabelId Name = Name##Id;
#define DO_LABEL(Name) DO_LABEL_STRUCT(kMem##Name)
#define DO_TEMP_LABEL(Name) DO_LABEL_STRUCT(kMem##Name)
#include "AllocatorLabelNames.h"
#undef DO_LABEL
#undef DO_TEMP_LABEL

#undef DO_LABEL_STRUCT


extern const char* const MemLabelName[];

#endif
