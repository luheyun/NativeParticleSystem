#include "PluginPrefix.h"
#include "VertexDeclarationD3D9.h"
#include "D3D9Context.h"
#include "GfxDevice/GfxDeviceTypes.h"
#include "Allocator/MemoryMacros.h"

VertexDeclarationD3D9::~VertexDeclarationD3D9()
{
	if (m_D3DDecl)
	{
		m_D3DDecl->Release();
	}
}

struct D3DVertexSemantics
{
	UInt8 usage;
	UInt8 index;
};

static D3DVertexSemantics kChannelVertexSemantics[] = // Expected size is kShaderChannelCount
{
	{ D3DDECLUSAGE_POSITION, 0 }, // position
	{ D3DDECLUSAGE_NORMAL,   0 }, // normal
	{ D3DDECLUSAGE_COLOR,    0 }, // color
	{ D3DDECLUSAGE_TEXCOORD, 0 }, // texcoord0
	{ D3DDECLUSAGE_TEXCOORD, 1 }, // texcoord1
	{ D3DDECLUSAGE_TEXCOORD, 2 }, // texcoord2
	{ D3DDECLUSAGE_TEXCOORD, 3 }, // texcoord3
#if GFX_HAS_TWO_EXTRA_TEXCOORDS
	{ D3DDECLUSAGE_TEXCOORD, 4 }, // texcoord4
	{ D3DDECLUSAGE_TEXCOORD, 5 }, // texcoord5
#endif
	{ D3DDECLUSAGE_TANGENT,  0 }, // tangent
};

