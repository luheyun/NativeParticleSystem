#ifndef VERTEXDECLARATION_H
#define VERTEXDECLARATION_H

#include "Graphics/Mesh/VertexData.h"

class EXPORT_COREMODULE VertexDeclaration
{
public:
	virtual ~VertexDeclaration() {}
};

// todo
class EXPORT_COREMODULE VertexDeclarationCache
{
public:
	virtual ~VertexDeclarationCache() { }
	virtual VertexDeclaration* GetVertexDecl(const VertexChannelsInfo& key);
	void Clear();
	void Commit();

protected:
	virtual VertexDeclaration* CreateVertexDeclaration(const VertexChannelsInfo& key) = 0;
	virtual void DestroyVertexDeclaration(VertexDeclaration* vertexDecl) = 0;

private:
	typedef std::map<VertexChannelsInfo, VertexDeclaration*> VertexDeclMap;
	VertexDeclMap m_VertexDeclMap;
	VertexDeclMap m_VertexDeclMapInterim;
};


#endif
