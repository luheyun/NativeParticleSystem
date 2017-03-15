#include "PluginPrefix.h"
#include "VertexData.h"

VertexData::~VertexData()
{
	Deallocate();
}

void VertexData::Deallocate()
{
	if (m_Data)
		delete m_Data;

	m_Data = NULL;
}