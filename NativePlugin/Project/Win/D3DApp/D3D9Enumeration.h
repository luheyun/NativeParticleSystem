#pragma once

#include "External/DirectX/include9/d3d9.h"
#include <vector>
#include "Include/Precomplie/PluginPrefix.h"

struct D3DDeviceCombo;

typedef std::vector<DWORD>						DwordVector;
typedef std::vector<D3DDeviceCombo>				D3DDeviceComboVector;


enum D3DVertexProcessing {
	kVPPureHardware,
	kVPHardware,
	kVPMixed,
	kVPSoftware,
};


//---------------------------------------------------------------------------

// A combo of adapter format, back buffer format, and windowed/fulscreen that
// is compatible with a D3D device.
struct D3DDeviceCombo {
public:
	// A depth/stencil buffer format that is incompatible with a multisample type.
	struct MultiSampleConflict {
		D3DFORMAT			format;
		D3DMULTISAMPLE_TYPE type;
	};
	typedef std::vector<MultiSampleConflict> MultiSampleConflictVector;
public:
	D3DFORMAT	adapterFormat;
	D3DFORMAT	backBufferFormat;
	bool		isWindowed;
	DWORD		presentationIntervals;

	DwordVector	depthStencilFormats;
	DwordVector	multiSampleTypes;
	MultiSampleConflictVector	conflicts;
};


//---------------------------------------------------------------------------

class D3D9FormatCaps {
public:
	D3D9FormatCaps() : m_VertexProcessings(0) { }

	bool	Enumerate( IDirect3D9& d3d );

	// Fills in BackBufferFormat, AutoDepthStencilFormat, PresentationInterval,
	// MultiSampleType, MultiSampleQuality.

	// Gets adapter format for doing CheckDeviceFormat checks.
	// Usually D3DFMT_X8R8G8B8, except for really old cards that can't do 32 bpp.
	D3DFORMAT GetAdapterFormatForChecks() const { return m_AdapterFormatForChecks; }

public:
	D3DDeviceComboVector	m_Combos;
	UInt32					m_VertexProcessings; // bitmask
	D3DFORMAT				m_AdapterFormatForChecks;
};
