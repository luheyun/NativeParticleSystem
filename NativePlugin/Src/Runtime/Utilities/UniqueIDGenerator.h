#pragma once

#include "Misc/AllocatorLabels.h"
#include "Utilities/dynamic_array.h"

struct UniqueSmallID
{
	UInt32 index	: 24;
	UInt32 version	: 8;

	UniqueSmallID () : index(0), version (0) { }

	friend bool operator == (UniqueSmallID lhs, UniqueSmallID rhs) { return *reinterpret_cast<UInt32*> (&lhs) == *reinterpret_cast<UInt32*> (&rhs); }
	friend bool operator != (UniqueSmallID lhs, UniqueSmallID rhs) { return *reinterpret_cast<UInt32*> (&lhs) != *reinterpret_cast<UInt32*> (&rhs); }
};

// Class to generate small reusable index. (And optionally also a version number for Exists checks)
// Uses simple free list algorithm.
// 0 is a reserved index and means NULL
class UniqueIDGenerator
{
public:
	UniqueIDGenerator(MemLabelId memLabel = kMemDynamicArray);

	// Allocate and deallocate identifier that has index + version.
	UniqueSmallID	CreateID();
	void			DestroyID( UniqueSmallID i_ID);

	// Returns whether or not the ID has been allocated and not yet been deallocated.
	//
	// Important Note:
	// We use an 8bit version number, thus if you call Deallocate and then call Exists,
	// then this function might return true, even though the id has been deallocated.
	// This is because the ID will get reused (after 255 allocations against the index)
	// So you can only use this functionality if the code relying on exists is not required to be reliably.
	// E.g. in the case of GeometryJob's the worst case is that we Sync one job too much,
	// but since this will be very very rare it is not a problem.
	bool		Exists( UniqueSmallID i_ID) const;

	// If you are not going to use the Exists function then you can use this simpler API without version'ing.
	UInt32		CreatePureIndex();
	void		DestroyPureIndex(UInt32 i_Index);

	void		DestroyAllIndices();

	size_t		GetArraySize () const { return m_IDs.size(); }

	// Deallocates all memory (Allocate / Deallocate after Cleanup is called is invalid)
	void		Cleanup ();

private:
	dynamic_array<UniqueSmallID>	m_IDs;
	UInt32						m_Free;
};
