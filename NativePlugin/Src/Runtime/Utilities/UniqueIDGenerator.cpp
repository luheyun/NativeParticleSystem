#include "PluginPrefix.h"
#include "UniqueIDGenerator.h"

UniqueIDGenerator::UniqueIDGenerator(MemLabelId i_MemLabel) : m_IDs(i_MemLabel)
{
	// Generated ID sequence should not contain 0
	// (0 is a reserved index for null)

	// Make sure that firstID has no valid index otherwise Exists (UniqueSmallID(0, 0)) will return true.
	// But since it means NULL, it should return false.
	// We just need to make sure index is not 0 here, any other number will work fine.
	UniqueSmallID firstID;
	firstID.index = 0xFFFFFF;
	firstID.version = 0;
	m_IDs.push_back(firstID);
	m_Free = 1;
}

void UniqueIDGenerator::Cleanup ()
{
	m_IDs.clear();
	// After cleanup is called, no more allocations are allowed to happen
	m_Free = 0xFFFFFFFF;
}

UniqueSmallID UniqueIDGenerator::CreateID ()
{
	// No free indices, expand maximum index size
	if (m_Free == m_IDs.size())
	{
		UniqueSmallID newID;
		newID.index = m_Free + 1;
		newID.version = 0;

		m_IDs.push_back(newID);
	}

	UInt32 index = m_Free;
	UInt32 version = m_IDs[index].version + 1;

	m_Free = m_IDs[index].index;
	m_IDs[index].index = index;
	m_IDs[index].version = version;

	UniqueSmallID result;
	result.index = index;
	result.version = version;

	return result;
}

void UniqueIDGenerator::DestroyID (UniqueSmallID i_ID)
{
	DestroyPureIndex (i_ID.index);
}

void UniqueIDGenerator::DestroyAllIndices()
{
	// Destroy all indices (Do not destroy the first ID, that is reserved)
	for (int i=1;i<m_IDs.size();i++)
	{
		if (m_IDs[i].index == i)
			DestroyPureIndex (i);
	}
}

UInt32 UniqueIDGenerator::CreatePureIndex()
{
	return CreateID ().index;
}

void UniqueIDGenerator::DestroyPureIndex(UInt32 i_Index)
{
	m_IDs[i_Index].index = m_Free;
	m_Free = i_Index;

	// m_IDs[i_index].index == _ID means it is in use (Exists function relies on it)
}

bool UniqueIDGenerator::Exists( UniqueSmallID i_ID) const
{
	return m_IDs[i_ID.index] == i_ID;
}
