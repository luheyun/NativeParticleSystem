#include "D3D9Enumeration.h"
#include "GfxDevice/d3d/D3D9Utils.h"

// ---------------------------------------------------------------------------


const int kMinDisplayWidth = 512;
const int kMinDisplayHeight = 384;
const int kMinColorBits = 4;
const int kMinAlphaBits = 0;

extern D3DDEVTYPE g_D3DDevType;
extern DWORD g_D3DAdapter;

// ---------------------------------------------------------------------------

static int GetFormatColorBits( D3DFORMAT fmt ) {
	switch( fmt ) {
	case D3DFMT_A2B10G10R10:
	case D3DFMT_A2R10G10B10:	return 10;
	case D3DFMT_R8G8B8:
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:		return 8;
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:		return 5;
	case D3DFMT_A4R4G4B4:
	case D3DFMT_X4R4G4B4:		return 4;
	case D3DFMT_R3G3B2:
	case D3DFMT_A8R3G3B2:		return 2;
	default:					return 0;
	}
}

static int GetFormatAlphaBits( D3DFORMAT fmt ) {
	switch( fmt ) {
	case D3DFMT_R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_R3G3B2:
	case D3DFMT_X4R4G4B4:		return 0;
	case D3DFMT_A8R8G8B8:
	case D3DFMT_A8R3G3B2:		return 8;
	case D3DFMT_A1R5G5B5:		return 1;
	case D3DFMT_A4R4G4B4:		return 4;
	case D3DFMT_A2B10G10R10:
	case D3DFMT_A2R10G10B10:	return 2;
	default:					return 0;
	}
}

int GetFormatDepthBits( D3DFORMAT fmt ) {
	switch( fmt ) {
	case D3DFMT_D16:		return 16;
	case D3DFMT_D15S1:		return 15;
	case D3DFMT_D24X8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:	return 24;
	case D3DFMT_D32:		return 32;
	default:				return 0;
	}
}

static D3DFORMAT ConvertToAlphaFormat( D3DFORMAT fmt )
{
	if( fmt == D3DFMT_X8R8G8B8 )
		fmt = D3DFMT_A8R8G8B8;
	else if( fmt == D3DFMT_X4R4G4B4 )
		fmt = D3DFMT_A4R4G4B4;
	else if( fmt == D3DFMT_X1R5G5B5 )
		fmt = D3DFMT_A1R5G5B5;
	return fmt;
}

// -----------------------------------------------------------------------------


static UInt32 buildVertexProcessings( const D3DCAPS9& caps )
{
	UInt32 result =  0;

	// TODO: check vertex shader version

	DWORD devCaps = caps.DevCaps;
	if( devCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) {
		if( devCaps & D3DDEVCAPS_PUREDEVICE ) {
			result |= (1<<kVPPureHardware);
		}
		result |= (1<<kVPHardware);
		result |= (1<<kVPMixed);
	}

	result |= (1<<kVPSoftware);

	return result;
}


static void buildDepthStencilFormats( IDirect3D9& d3d, D3DDeviceCombo& devCombo )
{
	const D3DFORMAT dsFormats[] = {
		D3DFMT_D24S8, D3DFMT_D24X8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D15S1, D3DFMT_D32,
	};
	const int dsFormatCount = sizeof(dsFormats) / sizeof(dsFormats[0]);
	
	for( int idsf = 0; idsf < dsFormatCount; ++idsf ) {
		D3DFORMAT format = dsFormats[idsf];
		if( SUCCEEDED( d3d.CheckDeviceFormat( g_D3DAdapter, g_D3DDevType, devCombo.adapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, format ) ) )
		{
			if( SUCCEEDED( d3d.CheckDepthStencilMatch( g_D3DAdapter, g_D3DDevType, devCombo.adapterFormat, devCombo.backBufferFormat, format ) ) )
			{
				devCombo.depthStencilFormats.push_back( format );
			}
		}
	}
}


static void buildMultiSampleTypes( IDirect3D9& d3d, D3DDeviceCombo& devCombo )
{
	const size_t kMaxSamples = 16;
	devCombo.multiSampleTypes.reserve( kMaxSamples );
	devCombo.multiSampleTypes.push_back( D3DMULTISAMPLE_NONE );

	for( int samples = 2; samples <= kMaxSamples; ++samples ) {
		D3DMULTISAMPLE_TYPE	msType = GetD3DMultiSampleType( samples );
		DWORD msQuality;
		if( SUCCEEDED( d3d.CheckDeviceMultiSampleType( g_D3DAdapter, g_D3DDevType, devCombo.backBufferFormat, devCombo.isWindowed, msType, NULL ) ) )
			devCombo.multiSampleTypes.push_back( samples );
	}
}


static void buildConflicts( IDirect3D9& d3d, D3DDeviceCombo& devCombo )
{
	for( size_t ids = 0; ids < devCombo.depthStencilFormats.size(); ++ids ) {
		D3DFORMAT format = (D3DFORMAT)devCombo.depthStencilFormats[ids];
		for( size_t ims = 0; ims < devCombo.multiSampleTypes.size(); ++ims ) {
			D3DMULTISAMPLE_TYPE msType = (D3DMULTISAMPLE_TYPE)devCombo.multiSampleTypes[ims];
			if( FAILED( d3d.CheckDeviceMultiSampleType(
				g_D3DAdapter, g_D3DDevType,
				format, devCombo.isWindowed, msType, NULL ) ) )
			{
				D3DDeviceCombo::MultiSampleConflict conflict;
				conflict.format = format;
				conflict.type = msType;
				devCombo.conflicts.push_back( conflict );
			}
		}
	}
}


static bool enumerateDeviceCombos( IDirect3D9& d3d, const D3DCAPS9& caps, const DwordVector& adapterFormats, D3DDeviceComboVector& outCombos )
{
	const D3DFORMAT bbufferFormats[] = {
		D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_A2R10G10B10,
		D3DFMT_R5G6B5, D3DFMT_A1R5G5B5, D3DFMT_X1R5G5B5
	};
	const int bbufferFormatCount = sizeof(bbufferFormats) / sizeof(bbufferFormats[0]);

	bool isWindowedArray[] = { false, true };

	// see which adapter formats are supported by this device
	for( size_t iaf = 0; iaf < adapterFormats.size(); ++iaf )
	{
		D3DFORMAT format = (D3DFORMAT)adapterFormats[iaf];
		for( int ibbf = 0; ibbf < bbufferFormatCount; ibbf++ )
		{
			D3DFORMAT bbufferFormat = bbufferFormats[ibbf];
			if( GetFormatAlphaBits(bbufferFormat) < kMinAlphaBits )
				continue;
			for( int iiw = 0; iiw < 2; ++iiw ) {
				bool isWindowed = isWindowedArray[iiw];
				if( FAILED( d3d.CheckDeviceType( g_D3DAdapter, g_D3DDevType, format, bbufferFormat, isWindowed ) ) )
					continue;

				// Here, we have an adapter format / backbuffer format/ windowed
				// combo that is supported by the system. We still need to find one or
				// more suitable depth/stencil buffer format, multisample type,
				// vertex processing type, and vsync.
				D3DDeviceCombo devCombo;

				devCombo.adapterFormat = format;
				devCombo.backBufferFormat = bbufferFormat;
				devCombo.isWindowed = isWindowed;
				devCombo.presentationIntervals = caps.PresentationIntervals;

				buildDepthStencilFormats( d3d, devCombo );
				if( devCombo.depthStencilFormats.empty() )
					continue;

				buildMultiSampleTypes( d3d, devCombo );
				if( devCombo.multiSampleTypes.empty() )
					continue;

				buildConflicts( d3d, devCombo );

				outCombos.push_back( devCombo );
			}
		}
	}

	return !outCombos.empty();
}


bool D3D9FormatCaps::Enumerate( IDirect3D9& d3d )
{
	HRESULT hr;

	const D3DFORMAT allowedFormats[] = {
		D3DFMT_X8R8G8B8, D3DFMT_X1R5G5B5, D3DFMT_R5G6B5, D3DFMT_A2R10G10B10
	};
	const int allowedFormatCount = sizeof(allowedFormats) / sizeof(allowedFormats[0]);

	m_AdapterFormatForChecks = D3DFMT_UNKNOWN;

	// build a list of all display adapter formats
	DwordVector adapterFormatList; // D3DFORMAT

	for( size_t ifmt = 0; ifmt < allowedFormatCount; ++ifmt )
	{
		D3DFORMAT format = allowedFormats[ifmt];
		int modeCount = d3d.GetAdapterModeCount( g_D3DAdapter, format );
		for( int mode = 0; mode < modeCount; ++mode ) {
			D3DDISPLAYMODE dm;
			d3d.EnumAdapterModes( g_D3DAdapter, format, mode, &dm );
			if( dm.Width < (UINT)kMinDisplayWidth || dm.Height < (UINT)kMinDisplayHeight || GetFormatColorBits(dm.Format) < kMinColorBits )
				continue;
			// adapterInfo->displayModes.push_back( dm );
			if( std::find(adapterFormatList.begin(),adapterFormatList.end(),dm.Format) == adapterFormatList.end() ) {
				adapterFormatList.push_back( dm.Format );
				if( m_AdapterFormatForChecks == D3DFMT_UNKNOWN )
					m_AdapterFormatForChecks = format;
			}
		}
	}

	if( m_AdapterFormatForChecks == D3DFMT_UNKNOWN ) // for some reason no format was selected for checks, use default
		m_AdapterFormatForChecks = allowedFormats[0];

	// get info for device on this adapter
	D3DCAPS9 caps;
	if( FAILED( d3d.GetDeviceCaps( g_D3DAdapter, g_D3DDevType, &caps ) ) )
		return false;

	// find suitable vertex processing modes (if any)
	m_VertexProcessings = buildVertexProcessings( caps );

	// get info for each device combo on this device
	if( !enumerateDeviceCombos( d3d, caps, adapterFormatList, m_Combos ) )
		return false;

	return true;
}
