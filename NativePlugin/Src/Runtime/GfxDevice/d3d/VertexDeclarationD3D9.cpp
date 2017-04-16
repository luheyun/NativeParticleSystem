#include "PluginPrefix.h"
#include "VertexDeclarationD3D9.h"
#include "D3D9Context.h"
#include "GfxDevice/GfxDeviceTypes.h"

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
	{ D3DDECLUSAGE_NORMAL, 0 }, // normal
	{ D3DDECLUSAGE_COLOR, 0 }, // color
	{ D3DDECLUSAGE_TEXCOORD, 0 }, // texcoord0
	{ D3DDECLUSAGE_TEXCOORD, 1 }, // texcoord1
	{ D3DDECLUSAGE_TEXCOORD, 2 }, // texcoord2
	{ D3DDECLUSAGE_TEXCOORD, 3 }, // texcoord3
#if GFX_HAS_TWO_EXTRA_TEXCOORDS
	{ D3DDECLUSAGE_TEXCOORD, 4 }, // texcoord4
	{ D3DDECLUSAGE_TEXCOORD, 5 }, // texcoord5
#endif
	{ D3DDECLUSAGE_TANGENT, 0 }, // tangent
};

static FORCE_INLINE D3DDECLTYPE GetD3DVertexDeclType(const ChannelInfo& info)
{
	switch (info.format)
	{
	case kChannelFormatFloat:
	{
		switch (info.dimension)
		{
		case 1: return D3DDECLTYPE_FLOAT1;
		case 2: return D3DDECLTYPE_FLOAT2;
		case 3: return D3DDECLTYPE_FLOAT3;
		case 4: return D3DDECLTYPE_FLOAT4;
		}
		break;
	}
	case kChannelFormatFloat16:
	{
		switch (info.dimension)
		{
		case 2: return D3DDECLTYPE_FLOAT16_2;
		case 4: return D3DDECLTYPE_FLOAT16_4;
		}
		break;
	}
	case kChannelFormatColor:
	{
		return D3DDECLTYPE_D3DCOLOR;
	}
	}
	return D3DDECLTYPE_UNUSED;
}

VertexDeclaration* VertexDeclarationCacheD3D9::CreateVertexDeclaration(const VertexChannelsInfo& key)
{
	// KD: not sure if elements need to be ordered by stream, playing it safe
	D3DVERTEXELEMENT9 elements[kShaderChannelCount + 1];
	int elIndex = 0;
	for (int stream = 0; stream < kMaxVertexStreams; stream++)
	{
		for (int chan = 0; chan < kShaderChannelCount; chan++)
		{
			if (key.channels[chan].stream == stream && key.channels[chan].IsValid())
			{
				D3DVERTEXELEMENT9& elem = elements[elIndex++];
				elem.Stream = stream;
				elem.Offset = key.channels[chan].offset;
				elem.Type = GetD3DVertexDeclType(key.channels[chan]);
				elem.Method = D3DDECLMETHOD_DEFAULT;
				elem.Usage = kChannelVertexSemantics[chan].usage;
				elem.UsageIndex = kChannelVertexSemantics[chan].index;
			}
		}
	}
	D3DVERTEXELEMENT9 declEnd = D3DDECL_END();
	elements[elIndex] = declEnd;

	IDirect3DVertexDeclaration9* d3dDecl = NULL;
	HRESULT hr = GetD3DDevice()->CreateVertexDeclaration(elements, &d3dDecl);
	if (FAILED(hr)) {
		// TODO: error!
	}
	return new VertexDeclarationD3D9(d3dDecl);
}

void VertexDeclarationCacheD3D9::DestroyVertexDeclaration(VertexDeclaration* vertexDecl)
{
	if (vertexDecl != nullptr)
		delete vertexDecl;
}
