#pragma once

#include "GfxDevice/GfxBuffer.h"
#include "D3D9Includes.h"

// Implements Direct3D9 index buffer
class IndexBufferD3D9 : public IndexBuffer
{
public:
	IndexBufferD3D9();
	virtual ~IndexBufferD3D9();

	void	UpdateIndexBuffer(GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* indices, UInt32 flags);
	void*	BeginWriteIndices(size_t offset, size_t size);
	void	EndWriteIndices();

	virtual bool IsLost() const;
	virtual void Reset();

	int		GetRuntimeMemorySize() const;

	IDirect3DIndexBuffer9* GetD3DIB() { return m_D3DIB; }

private:
	IDirect3DIndexBuffer9* m_D3DIB;
};
