#pragma once

#include "GfxDevice/GfxBuffer.h"

GfxBuffer* GetRealBuffer(GfxBuffer* threadedBuffer);
GfxBuffer* GetRealBufferOrNull(GfxBuffer* threadedBuffer);
void GetRealBufferStreamSourceArray(VertexStreamSource* outArray, const VertexStreamSource* inArray, size_t count);

class ThreadedBuffer : public GfxBuffer
{
public:
	ThreadedBuffer(GfxBufferTarget target) : GfxBuffer(target), m_RealBuffer(NULL)
	{
#if GFX_BUFFERS_CAN_BECOME_LOST
		m_Lost = false;
#endif
	}

#if GFX_BUFFERS_CAN_BECOME_LOST
	virtual bool IsLost() const			{ return m_Lost; }
	virtual void Reset()				{ m_Lost = true; }
#endif

private:
	friend class GfxDeviceClient;
	friend class GfxDeviceWorker;

	void Update(GfxBufferMode mode, GfxBufferLabel label, size_t size)
	{
		m_Mode = mode;
		m_Label = label;
		m_BufferSize = size;
#if GFX_BUFFERS_CAN_BECOME_LOST
		m_Lost = false;
#endif
	}

	friend GfxBuffer* GetRealBuffer(GfxBuffer* threadedBuffer);
	friend GfxBuffer* GetRealBufferOrNull(GfxBuffer* threadedBuffer);

	struct WriteInfo
	{
		WriteInfo() : buffer(NULL), offset(0), size(0) {}
		void* buffer;
		size_t offset;
		size_t size;
	};

	WriteInfo m_WriteInfo;
	GfxBuffer* volatile m_RealBuffer;

#if GFX_BUFFERS_CAN_BECOME_LOST
	bool m_Lost;
#endif
};

inline GfxBuffer* GetRealBuffer(GfxBuffer* threadedBuffer)
{
	return static_cast<ThreadedBuffer*>(threadedBuffer)->m_RealBuffer;
}

inline GfxBuffer* GetRealBufferOrNull(GfxBuffer* threadedBuffer)
{
	return threadedBuffer ? static_cast<ThreadedBuffer*>(threadedBuffer)->m_RealBuffer : NULL;
}

inline void GetRealBufferStreamSourceArray(VertexStreamSource* outArray, const VertexStreamSource* inArray, size_t count)
{
	for (size_t vb = 0; vb < count; vb++)
	{
		outArray[vb].buffer = GetRealBuffer(inArray[vb].buffer);
		outArray[vb].stride = inArray[vb].stride;
	}
}
