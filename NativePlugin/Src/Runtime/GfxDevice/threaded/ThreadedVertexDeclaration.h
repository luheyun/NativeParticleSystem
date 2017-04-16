#pragma once

#include "Runtime/GfxDevice/VertexDeclaration.h"
#include "Runtime/Threads/AtomicOps.h"

class ThreadedVertexDeclaration : public VertexDeclaration
{
public:
	ThreadedVertexDeclaration(const VertexChannelsInfo& key) : m_RealVertexDeclaration(NULL), m_Key(key) {}
private:
	friend VertexDeclaration* GetRealDecl(GfxDevice* device, VertexDeclaration* threadedVertDecl);

	VertexDeclaration* m_RealVertexDeclaration;
	VertexChannelsInfo m_Key;
};

class ThreadedVertexDeclarationCache : public VertexDeclarationCache
{
public:
	virtual VertexDeclaration* GetVertexDecl(const VertexChannelsInfo& key)
	{
		return VertexDeclarationCache::GetVertexDecl(key);
	}
    
    ThreadedVertexDeclarationCache(): m_RefCount(1) {}
    
    void AddRef()
    {
        AtomicIncrement(&m_RefCount);
    }
    
    bool Release(MemLabelId label)
    {
        if (AtomicDecrement(&m_RefCount) == 0)
        {
            this->~ThreadedVertexDeclarationCache();
            UNITY_FREE(label, this);
            return true;
        }
        else
        	return false;
    }

protected:
    virtual ~ThreadedVertexDeclarationCache()
    {
        Clear();
    }
    
	virtual VertexDeclaration* CreateVertexDeclaration(const VertexChannelsInfo& key)
	{
		return UNITY_NEW(ThreadedVertexDeclaration, kMemGfxThread)(key);
	}
	
	virtual void DestroyVertexDeclaration(VertexDeclaration* vertexDecl)
	{
		UNITY_DELETE(vertexDecl, kMemGfxThread);
	}
    
    volatile int m_RefCount;
};

inline VertexDeclaration* GetRealDecl(GfxDevice* device, VertexDeclaration* threadedVertDecl)
{
	ThreadedVertexDeclaration* vdecl = static_cast<ThreadedVertexDeclaration*>(threadedVertDecl);
	if (!vdecl->m_RealVertexDeclaration)
		vdecl->m_RealVertexDeclaration = device->GetVertexDeclaration(vdecl->m_Key);

	return vdecl->m_RealVertexDeclaration;
}
