#include "PluginPrefix.h"
#include "D3D9Utils.h"
//#include "Utilities/ArrayUtility.h"
//#include "Shaders/GraphicsCaps.h"


struct D3D9Error {
	HRESULT hr;
	const char* message;
};

static D3D9Error s_D3DErrors[] = {
	{ D3DOK_NOAUTOGEN, "no mipmap autogen" },
	{ D3DERR_WRONGTEXTUREFORMAT, "wrong texture format" },
	{ D3DERR_UNSUPPORTEDCOLOROPERATION, "unsupported color op" },
	{ D3DERR_UNSUPPORTEDCOLORARG, "unsupported color arg" },
	{ D3DERR_UNSUPPORTEDALPHAOPERATION, "unsupported alpha op" },
	{ D3DERR_UNSUPPORTEDALPHAARG, "unsupported alpha arg" },
	{ D3DERR_TOOMANYOPERATIONS, "too many texture operations" },
	{ D3DERR_CONFLICTINGTEXTUREFILTER, "conflicting texture filters" },
	{ D3DERR_UNSUPPORTEDFACTORVALUE, "unsupported factor value" },
	{ D3DERR_CONFLICTINGRENDERSTATE, "conflicting render states" },
	{ D3DERR_UNSUPPORTEDTEXTUREFILTER, "unsupported texture filter" },
	{ D3DERR_CONFLICTINGTEXTUREPALETTE, "conflicting texture palettes" },
	{ D3DERR_DRIVERINTERNALERROR, "internal driver error" },
	{ D3DERR_NOTFOUND, "requested item not found" },
	{ D3DERR_MOREDATA, "more data than fits into buffer" },
	{ D3DERR_DEVICELOST, "device lost" },
	{ D3DERR_DEVICENOTRESET, "device not reset" },
	{ D3DERR_NOTAVAILABLE, "queried technique not available" },
	{ D3DERR_OUTOFVIDEOMEMORY, "out of VRAM" },
	{ D3DERR_INVALIDDEVICE, "invalid device" },
	{ D3DERR_INVALIDCALL, "invalid call" },
	{ D3DERR_DRIVERINVALIDCALL, "driver invalid call" },
	{ D3DERR_WASSTILLDRAWING, "was still drawing" },
	{ S_OK, "S_OK" },
	{ E_FAIL, "E_FAIL" },
	{ E_INVALIDARG, "E_INVALIDARG" },
	{ E_OUTOFMEMORY, "out of memory" },
};

//const char* GetD3D9Error( HRESULT hr )
//{
//	for( int i = 0; i < ARRAY_SIZE(s_D3DErrors); ++i )
//	{
//		if( hr == s_D3DErrors[i].hr )
//			return s_D3DErrors[i].message;
//	}
//
//	static char buffer[1000];
//	sprintf( buffer, "unknown error, code 0x%X", hr );
//	return buffer;
//}

int GetBPPFromD3DFormat( D3DFORMAT format )
{
	switch( format ) {
	case D3DFMT_UNKNOWN:
	case kD3D9FormatNULL:
		return 0;
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8R8G8B8:
	case D3DFMT_A2R10G10B10:
	case D3DFMT_A2B10G10R10:
	case D3DFMT_R8G8B8:
	case D3DFMT_A8B8G8R8:
	case D3DFMT_R32F:
	case D3DFMT_G16R16F:
	case D3DFMT_D24X8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case kD3D9FormatINTZ:
		return 32;
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A4R4G4B4:
	case D3DFMT_X4R4G4B4:
	case D3DFMT_R5G6B5:
	case D3DFMT_R16F:
	case D3DFMT_D16:
	case D3DFMT_D15S1:
	case D3DFMT_D16_LOCKABLE:
	case D3DFMT_L16:
	case D3DFMT_A8L8:
	case kD3D9FormatDF16:
		return 16;
	case D3DFMT_A16B16G16R16F:
	case D3DFMT_G32R32F:
		return 64;
	case D3DFMT_A32B32G32R32F:
		return 128;
	case D3DFMT_DXT1:
		return 4;
	case D3DFMT_A8:
	case D3DFMT_L8:
	case D3DFMT_DXT3:
	case D3DFMT_DXT5:
		return 8;
	default:
		//ErrorString( Format("Unknown D3D format %x", format) );
		return 32;
	}
}

int GetStencilBitsFromD3DFormat (D3DFORMAT fmt)
{
	switch( fmt ) {
	case D3DFMT_D15S1:		return 1;
	case D3DFMT_D24S8:
	case kD3D9FormatINTZ:	return 8;
	case D3DFMT_D24X4S4:	return 4;
	default:				return 0;
	}
}

D3DMULTISAMPLE_TYPE GetD3DMultiSampleType (int samples)
{
	// Optimizer should take care of this, since value of D3DMULTISAMPLE_N_SAMPLES is N
	switch( samples ) {
	case 0:
	case 1: return D3DMULTISAMPLE_NONE;
	case 2: return D3DMULTISAMPLE_2_SAMPLES;
	case 3: return D3DMULTISAMPLE_3_SAMPLES;
	case 4: return D3DMULTISAMPLE_4_SAMPLES;
	case 5: return D3DMULTISAMPLE_5_SAMPLES;
	case 6: return D3DMULTISAMPLE_6_SAMPLES;
	case 7: return D3DMULTISAMPLE_7_SAMPLES;
	case 8: return D3DMULTISAMPLE_8_SAMPLES;
	case 9: return D3DMULTISAMPLE_9_SAMPLES;
	case 10: return D3DMULTISAMPLE_10_SAMPLES;
	case 11: return D3DMULTISAMPLE_11_SAMPLES;
	case 12: return D3DMULTISAMPLE_12_SAMPLES;
	case 13: return D3DMULTISAMPLE_13_SAMPLES;
	case 14: return D3DMULTISAMPLE_14_SAMPLES;
	case 15: return D3DMULTISAMPLE_15_SAMPLES;
	case 16: return D3DMULTISAMPLE_16_SAMPLES;
	default:
		//ErrorString("Unknown sample count");
		return D3DMULTISAMPLE_NONE;
	}
}

bool CheckD3D9DebugRuntime (IDirect3DDevice9* dev)
{
	IDirect3DQuery9* query = NULL;
	HRESULT hr = dev->CreateQuery (D3DQUERYTYPE_VERTEXSTATS, &query);
	if( SUCCEEDED(hr) )
	{
		query->Release ();
		return true;
	}
	return false;
}


D3D9DepthStencilTexture CreateDepthStencilTextureD3D9 (IDirect3DDevice9* dev, int width, int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE msType, DWORD msQuality, BOOL discardable)
{
	D3D9DepthStencilTexture tex;

	HRESULT hr = dev->CreateDepthStencilSurface (width, height, format, msType, msQuality, discardable, &tex.m_Surface, NULL);
	
	//if (tex.m_Surface)
		//REGISTER_EXTERNAL_GFX_ALLOCATION_REF(tex.m_Surface, width * height * GetBPPFromD3DFormat(format), NULL);

	return tex;
}
