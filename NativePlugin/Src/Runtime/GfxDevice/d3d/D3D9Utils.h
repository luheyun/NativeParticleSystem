#pragma once

#include "D3D9Includes.h"
#include "GfxDevice/GfxDeviceTypes.h"

#define D3D9_CALL(x) x

const char* GetD3D9Error( HRESULT hr );
int GetBPPFromD3DFormat( D3DFORMAT format );
int GetStencilBitsFromD3DFormat (D3DFORMAT fmt);
EXPORT_COREMODULE D3DMULTISAMPLE_TYPE GetD3DMultiSampleType (int samples);

bool CheckD3D9DebugRuntime (IDirect3DDevice9* dev);

struct D3D9DepthStencilTexture {
	D3D9DepthStencilTexture() : m_Texture(NULL), m_Surface(NULL) {}

	IDirect3DTexture9*	m_Texture;
	IDirect3DSurface9*	m_Surface;

	void Release() {
		if (m_Texture) {
			m_Texture->Release();
			m_Texture = NULL;
		}
		if (m_Surface) {
			m_Surface->Release();
			m_Surface = NULL;
		}
	}
};

const D3DFORMAT kD3D9FormatDF16 = (D3DFORMAT)MAKEFOURCC('D','F','1','6');
const D3DFORMAT kD3D9FormatINTZ = (D3DFORMAT)MAKEFOURCC('I','N','T','Z');
const D3DFORMAT kD3D9FormatNULL = (D3DFORMAT)MAKEFOURCC('N','U','L','L');
const D3DFORMAT kD3D9FormatRESZ = (D3DFORMAT)MAKEFOURCC('R','E','S','Z');
const D3DFORMAT kD3D9FormatATOC = (D3DFORMAT)MAKEFOURCC('A','T','O','C');


D3D9DepthStencilTexture CreateDepthStencilTextureD3D9 (
	IDirect3DDevice9* dev, int width, int height, D3DFORMAT format,
	D3DMULTISAMPLE_TYPE msType, DWORD msQuality, BOOL discardable );

template <typename Buffer>
inline void ReleaseD3D9Buffer(Buffer*& buffer)
{
	if (buffer != NULL)
	{
		ULONG refCount = buffer->Release();
		buffer = NULL;
	}
}

static inline DWORD GetD3D9BufferUsage (GfxBufferMode mode)
{
	switch (mode)
	{
	case kGfxBufferModeImmutable:
	case kGfxBufferModeStreamOut:
		return 0;
	case kGfxBufferModeDynamic:
	case kGfxBufferModeCircular:
		return D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	default:
		return 0;
	}
}

static inline D3DPOOL GetD3D9BufferPool (GfxBufferMode mode)
{
	switch (mode)
	{
	case kGfxBufferModeImmutable:
	case kGfxBufferModeStreamOut:
		return D3DPOOL_MANAGED;
	case kGfxBufferModeDynamic:
	case kGfxBufferModeCircular:
		return D3DPOOL_DEFAULT;
	default:
		return D3DPOOL_MANAGED;
	}
}

static inline DWORD GetD3D9BufferLockFlags (GfxBufferMode mode, UInt32 offset)
{
	switch (mode)
	{
	case kGfxBufferModeImmutable:
	case kGfxBufferModeStreamOut:
		return 0;
	case kGfxBufferModeDynamic:
		return D3DLOCK_DISCARD;
	case kGfxBufferModeCircular:
		// Discard and synchronize with GPU on rewind to buffer start
		// Otherwise append and promise not to touch data we've drawn
		return (offset == 0) ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE;
	default:
		return 0;
	}
}
