#pragma once

#include "Utilities/LinkedList.h"
#include "GfxDevice/GfxDeviceTypes.h"

class EXPORT_COREMODULE GfxBuffer : public ListElement
{
public:
	virtual ~GfxBuffer() {}

	size_t	GetBufferSize() const		{ return m_BufferSize; }
	GfxBufferTarget GetTarget() const	{ return m_Target; }
	GfxBufferMode GetMode() const		{ return m_Mode; }
	GfxBufferLabel GetLabel() const		{ return m_Label; }
	bool	IsMappable() const			{ return m_Mode == kGfxBufferModeDynamic || m_Mode == kGfxBufferModeCircular; }

	virtual bool IsLost() const			{ return false; }
	virtual void Reset() {}

protected:
	GfxBuffer(GfxBufferTarget target) : m_Target(target), m_Mode(kGfxBufferModeImmutable), m_Label(kGfxBufferLabelDefault), m_BufferSize(0) {}

	GfxBufferTarget m_Target;
	GfxBufferMode m_Mode;
	GfxBufferLabel m_Label;
	size_t m_BufferSize;
};

class IndexBuffer : public GfxBuffer
{
protected:
	IndexBuffer() : GfxBuffer(kGfxBufferTargetIndex) {}
};

class VertexBuffer : public GfxBuffer
{
protected:
	VertexBuffer() : GfxBuffer(kGfxBufferTargetVertex) {}
};