
#pragma once

#include "Runtime/GfxDevice/GfxDeviceConfigure.h"
#include "Runtime/GfxDevice/VertexDeclaration.h"
#include "BuffersGLES.h"
#include <cstring>

enum VertexAttributeGLES
{
	kVertexArrayGLES = 0,
	kColorArrayGLES,
	kNormalArrayGLES,
	kTexCoord0ArrayGLES,
};


class VertexDeclarationGLES : public VertexDeclaration
{
public:
	VertexDeclarationGLES(const ChannelInfoArray& channels)
	{
		std::memcpy(&m_Channels, &channels, sizeof(channels));
	}
	VertexDeclarationGLES() {}

	ChannelInfoArray m_Channels;
};

class VertexDeclarationCacheGLES : public VertexDeclarationCache
{
protected:
	virtual VertexDeclaration* CreateVertexDeclaration(const VertexChannelsInfo& key)
	{
		return UNITY_NEW(VertexDeclarationGLES, kMemGfxDevice)(key.channels);
	}
	virtual void DestroyVertexDeclaration(VertexDeclaration* vertexDecl)
	{
		UNITY_DELETE(vertexDecl, kMemGfxDevice);
	}
};

struct VertexStreamSource;
class VertexDeclarationGLES;
class ChannelAssigns;
void SetVertexStateGLES(const ChannelAssigns& channels, VertexDeclarationGLES* decl, const VertexStreamSource* vertexStreams, UInt32 firstVertex, size_t vertexStreamCount, size_t vertexCount = 0);
