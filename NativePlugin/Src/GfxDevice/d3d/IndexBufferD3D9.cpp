#include "PluginPrefix.h"
#include "GfxDeviceD3D9.h"
#include "IndexBufferD3D9.h"
#include "D3D9Context.h"
#include "D3D9Utils.h"

IndexBufferD3D9::IndexBufferD3D9()
	: m_D3DIB(NULL)
{
}

IndexBufferD3D9::~IndexBufferD3D9()
{
	ReleaseD3D9Buffer(m_D3DIB);
}

void IndexBufferD3D9::UpdateIndexBuffer(GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* indices, UInt32 flags)
{
	m_Label = label; // Always set label

	if (m_D3DIB == NULL || size != m_BufferSize || mode != m_Mode)
	{
		m_Mode = mode;
		m_BufferSize = 0;
		ReleaseD3D9Buffer(m_D3DIB);
		// D3DUSAGE_WRITEONLY only affects the performance of D3DPOOL_DEFAULT buffers
		DWORD usage = GetD3D9BufferUsage(m_Mode);
		D3DPOOL pool = GetD3D9BufferPool(m_Mode);
		HRESULT hr = GetD3DDevice()->CreateIndexBuffer(size, usage, D3DFMT_INDEX16, pool, &m_D3DIB, NULL);
		if (FAILED(hr))
		{
			//printf_console( "D3D9: failed to create index buffer of size %d [%s]\n", size, GetD3D9Error(hr) );
			return;
		}
		REGISTER_EXTERNAL_GFX_ALLOCATION_REF(m_D3DIB, size, this);
		m_BufferSize = size;
	}

	// Don't update contents if there is no data. We allow creating an empty buffer that is written to later.
	if (indices)
	{
		void* buffer = BeginWriteIndices(0, 0); // Lock entire buffer
		if (buffer)
		{
			memcpy(buffer, indices, size);
			EndWriteIndices();
		}
	}
}

void* IndexBufferD3D9::BeginWriteIndices(size_t offset, size_t size)
{
	if (m_D3DIB == NULL)
	{
		//printf_console( "D3D9: attempt to lock null index buffer\n" );
		return NULL;
	}

	if (size == 0)
		size = m_BufferSize;

	void* buffer = NULL;
	DWORD lockFlags = GetD3D9BufferLockFlags(m_Mode, offset);
	HRESULT hr = m_D3DIB->Lock(offset, size, &buffer, lockFlags);
	if (FAILED (hr))
	{
		//printf_console("D3D9: failed to lock index buffer %p of size %i [%s]\n", m_D3DIB, m_BufferSize, GetD3D9Error(hr));
		return NULL;
	}

	return buffer;
}

void IndexBufferD3D9::EndWriteIndices()
{
	HRESULT hr = m_D3DIB->Unlock();
	//if (FAILED(hr))
	//	printf_console("D3D9: failed to unlock index buffer %p of size %i [%s]\n", m_D3DIB, m_BufferSize, GetD3D9Error(hr));
}

bool IndexBufferD3D9::IsLost() const
{
	return m_BufferSize && !m_D3DIB;
}

void IndexBufferD3D9::Reset()
{
	ReleaseD3D9Buffer(m_D3DIB);
}

int IndexBufferD3D9::GetRuntimeMemorySize() const
{
	return m_BufferSize;
}
