#include "PluginPrefix.h"
#include "GfxDeviceD3D9.h"
#include "VertexBufferD3D9.h"
#include "D3D9Context.h"
#include "D3D9Utils.h"

VertexBufferD3D9::VertexBufferD3D9() : m_D3DVB(NULL)
{
}

VertexBufferD3D9::~VertexBufferD3D9()
{
	ReleaseD3D9Buffer(m_D3DVB);
}

void VertexBufferD3D9::Update(GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* vertices)
{
	m_Label = label; // Always set label

	if (m_D3DVB == NULL || size != m_BufferSize || mode != m_Mode)
	{
		m_Mode = mode;
		m_BufferSize = 0;
		ReleaseD3D9Buffer(m_D3DVB);
		// D3DUSAGE_WRITEONLY only affects the performance of D3DPOOL_DEFAULT buffers
		DWORD usage = GetD3D9BufferUsage(m_Mode);
		D3DPOOL pool = GetD3D9BufferPool(m_Mode);
		HRESULT hr = GetD3DDevice()->CreateVertexBuffer(size, usage, 0, pool, &m_D3DVB, NULL);
		if (FAILED(hr))
		{
			//printf_console( "D3D9: failed to create vertex buffer of size %d [%s]\n", size, GetD3D9Error(hr) );
			return;
		}
		m_BufferSize = size;
	}

	// Don't update contents if there is no data. We allow creating an empty buffer that is written to later.
	if (vertices)
	{
		void* buffer = BeginWriteVertices(0, 0); // Lock entire buffer
		if (buffer)
		{
			memcpy(buffer, vertices, size);
			EndWriteVertices();
		}
	}
}

void* VertexBufferD3D9::BeginWriteVertices(size_t offset, size_t size)
{
	if (m_D3DVB == NULL)
	{
		//printf_console( "D3D9: attempt to lock null vertex buffer\n" );
		return NULL;
	}
	
	void* buffer = NULL;
	DWORD lockFlags = GetD3D9BufferLockFlags(m_Mode, offset);
	HRESULT hr = m_D3DVB->Lock(offset, size, &buffer, lockFlags);
	if (FAILED(hr))
	{
		//printf_console( "D3D9: failed to lock vertex buffer %p of size %i [%s]\n", m_D3DVB, m_BufferSize, GetD3D9Error(hr) );
		return NULL;
	}
	return buffer;
}

void VertexBufferD3D9::EndWriteVertices()
{
	HRESULT hr = m_D3DVB->Unlock();
	if (FAILED(hr))
	{
		//printf_console( "D3D9: failed to unlock vertex buffer %p of size %i [%s]\n", m_D3DVB, GetBufferSize(), GetD3D9Error(hr) );
	}
}

bool VertexBufferD3D9::IsLost() const
{
	return m_BufferSize && !m_D3DVB;
}

void VertexBufferD3D9::Reset()
{
	ReleaseD3D9Buffer(m_D3DVB);
}
