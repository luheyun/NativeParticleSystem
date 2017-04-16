#include "PluginPrefix.h"
#include "VertexDeclaration.h"

VertexDeclaration* VertexDeclarationCache::GetVertexDecl(const VertexChannelsInfo& key)
{
	VertexDeclaration* vertDecl = nullptr;

	VertexDeclMap::iterator it = m_VertexDeclMap.lower_bound(key);
	if (it == m_VertexDeclMap.end() || m_VertexDeclMap.key_comp()(key, it->first))
	{
		it = m_VertexDeclMapInterim.lower_bound(key);
		if (it == m_VertexDeclMapInterim.end() || m_VertexDeclMapInterim.key_comp()(key, it->first))
		{
			vertDecl = CreateVertexDeclaration(key);
			m_VertexDeclMapInterim.insert(it, VertexDeclMap::value_type(key, vertDecl));
		}
		else
		{
			vertDecl = it->second;
		}
	}
	else
	{
		vertDecl = it->second;
	}

	return vertDecl;
}

void VertexDeclarationCache::Clear()
{
	Commit();

	for (auto it = m_VertexDeclMap.begin(); it != m_VertexDeclMap.end(); ++it)
		DestroyVertexDeclaration(it->second);

	m_VertexDeclMap.clear();
}

void VertexDeclarationCache::Commit()
{
	auto it = m_VertexDeclMapInterim.begin();
	auto eit = m_VertexDeclMapInterim.end();

	while (it != eit)
	{
		m_VertexDeclMap.insert(*it++);
	}

	m_VertexDeclMapInterim.clear();
}