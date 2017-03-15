#pragma once

#include "GfxDevice/GfxBuffer.h"
#include "D3D9Includes.h"

// Implements Direct3D9 vertex buffer
class VertexBufferD3D9 : public VertexBuffer
{
public:
	VertexBufferD3D9();
	virtual ~VertexBufferD3D9();

	void	Update(GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* vertices);
	void*	BeginWriteVertices(size_t offset, size_t size);
	void	EndWriteVertices();

	virtual bool IsLost() const;
	virtual void Reset();

	IDirect3DVertexBuffer9* GetD3DVB() { return m_D3DVB; }

private:
	IDirect3DVertexBuffer9*	m_D3DVB;
};
