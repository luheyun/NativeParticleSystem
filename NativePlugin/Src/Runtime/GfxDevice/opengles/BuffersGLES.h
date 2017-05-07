#pragma once

#include "Runtime/GfxDevice/GfxBuffer.h"
#include "Runtime/Utilities/dynamic_array.h"
#include "Runtime/GfxDevice/opengles/DataBuffersGLES.h"
#include "Runtime/Shaders/GraphicsCaps.h"
#include "Runtime/GfxDevice/opengles/ApiConstantsGLES.h"

#if FORCE_DEBUG_BUILD_WEBGL
#	undef UNITY_WEBGL
#	define UNITY_WEBGL 1
#endif//FORCE_DEBUG_BUILD_WEBGL

// A template class that we can specialize per Index/VertexBuffer to map GfxBufferMode to DataBuffer usage types
template <class TBuffer> struct GfxBufferModeToDataBufferUsageMapper
{
	// Empty impl that doesn't return anything. Causes error; forces template specialization for any actual use.
//	static DataBufferGLES::BufferUsage map(const GfxBufferMode &in)
};

// Usage maps for index and vertex buffers.
struct IndexBufferUsageMapperGLES
{
	static DataBufferGLES::BufferUsage map(const GfxBufferMode &in)
	{
		switch (in)
		{
		case kGfxBufferModeImmutable:
			return DataBufferGLES::kStaticIBO;
		case kGfxBufferModeDynamic:
			return DataBufferGLES::kDynamicIBO;
		case kGfxBufferModeCircular:
			return DataBufferGLES::kCircularIBO;
		default:
			return DataBufferGLES::kDynamicIBO;
		}
	}
};

struct VertexBufferUsageMapperGLES
{
	static DataBufferGLES::BufferUsage map(const GfxBufferMode &in)
	{
		switch (in)
		{
		case kGfxBufferModeImmutable:
			return DataBufferGLES::kStaticVBO;
		case kGfxBufferModeDynamic:
			return DataBufferGLES::kDynamicVBO;
		case kGfxBufferModeCircular:
			return DataBufferGLES::kCircularVBO;
		case kGfxBufferModeStreamOut:
			return DataBufferGLES::kTFDestination;
		default:
			return DataBufferGLES::kDynamicVBO;
		}
	}
};



template <class TBuffer, class UsageMapper> class DrawBufferGLES : public TBuffer
{
public:
	DrawBufferGLES() : TBuffer(), m_Buffer(0), m_IsMapped(false)
	{
		TBuffer::m_BufferSize = 0;
		m_Usage = UsageMapper::map(TBuffer::m_Mode);
	}

	virtual ~DrawBufferGLES()
	{
		if (m_Buffer)
			m_Buffer->Release();
	}

	// Update/Recreate the buffer. data may be null to just initialize the storage.
	void	Update(GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data)
	{
		// Label for bookkeeping of memory use
		TBuffer::m_Label = label;
		if (mode != TBuffer::m_Mode)
		{
			// Usage has changed, release buffer
			TBuffer::m_Mode = mode;
			m_Usage = UsageMapper::map(TBuffer::m_Mode);
			if (m_Buffer)
			{
				m_Buffer->Release();
			}
			m_Buffer = 0;
		}
		// Store the buffer size. Note that this may be smaller than the actual buffer we get from buffermanager
		TBuffer::m_BufferSize = size;

		EnsureBuffer(size);

		m_Buffer->Upload(0, size, data);
		
	}

	void*	BeginWrite(size_t offset, size_t size)
	{
		void *res = 0;

		// size can be zero, means that the whole buffer should be mapped.
		// or at least as much as the client thinks the buffer size is.
		if (size == 0)
			size = TBuffer::m_BufferSize - offset;

		bool useMapBuffer = GetGraphicsCaps().gles.hasMapbufferRange;

		m_IsMapped = useMapBuffer;
		if (useMapBuffer)
		{
			UInt32 flags = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT;

			switch(TBuffer::m_Mode)
			{
				default:
					DebugAssertMsg(0, "Attempting to map a non-dynamic buffer");
					return NULL;
				case kGfxBufferModeCircular:
					// If we're rewinding the buffer, check for need for recycling.
					if(offset == 0)
					{
						EnsureBuffer(TBuffer::m_BufferSize);
#if UNITY_OSX && UNITY_EDITOR
						// Remove the unsynchronized flag on nvidia osx editor when rewinding,
						// there's something wrong in the drivers when multiple contexts are involved
						if(GetGraphicsCaps().gles.isNvidiaGpu)
							flags = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
#endif
					}
					
					// This is buggy on at least OSX and Linux NVidias (GTX 650)
					// This is slow (~2x) with AMD on OSX (https://confluence.hq.unity3d.com/display/DEV/Projects+for+Testing+Rendering+CPU+Performance, HeavyMaterialAnimation)
					// This is slightly faster (+10%) on Intel (HeavyMaterialAnimation)
					// Performance on Windows and Linux are untested.
					if((UNITY_OSX && GetGraphicsCaps().gles.isIntelGpu) || UNITY_WIN)
						flags |= GL_MAP_INVALIDATE_RANGE_BIT;
					
					break;
				case kGfxBufferModeDynamic:
					EnsureBuffer(offset + size);
					flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
#if UNITY_OSX && UNITY_EDITOR
					// Remove the unsynchronized and invalidate buffer flags on nvidia osx editor,
					// there's something wrong in the drivers when multiple contexts are involved
					if(GetGraphicsCaps().gles.isNvidiaGpu)
						flags = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
#endif
					break;
			}
			
			m_WriteOffset = offset;

			res = m_Buffer->Map(offset, size, flags);
		}
		else
		{
			m_BufferData.resize_uninitialized(TBuffer::m_BufferSize);
			res = (void *) (m_BufferData.data()+offset);
			m_WriteOffset = offset;
		}
		return res;
	}

	void	EndWrite(size_t bytesWritten)
	{
		if (m_IsMapped)
		{
			Assert(m_Buffer);
			Assert(m_Buffer->GetSize() >= bytesWritten);

			m_Buffer->FlushMappedRange(0, bytesWritten);
			m_Buffer->Unmap();
			m_IsMapped = false;
		}
		else
		{
			Assert(m_BufferData.capacity() >= m_WriteOffset + bytesWritten);

			// When not mapping, we can delay the buffer creation until here,
			// we may be able to use smaller buffer.
			EnsureBuffer(m_WriteOffset + bytesWritten);

			m_Buffer->Upload(m_WriteOffset, bytesWritten, m_BufferData.data());

			// An educated guess: release the static buffer immediately, in other cases leave it in place
			// It is likely that dynamic buffers will be written to again, and we'll need this again.
			if (TBuffer::m_Mode == kGfxBufferModeImmutable)
				m_BufferData.clear();
		}
	}

	// Retrieve the currently active underlying GL buffer object
	GLuint GetGLName()
	{
		Assert(m_Buffer);
		return m_Buffer->GetBuffer();
	}

	const GLvoid* GetBindPointer(UInt32 offset) const
	{
		return (GLvoid *)(uintptr_t)offset;
	}

	DataBufferGLES * GetActiveBuffer()
	{
		Assert(m_Buffer);
		return m_Buffer;
	}

	void RecordRender()
	{
		Assert(m_Buffer);
		m_Buffer->RecordRender();
	}

	// Get a non-stalling buffer we can write to
	DataBufferGLES *GetGPUSkinningTarget()
	{
		Assert(m_Usage == DataBufferGLES::kTFDestination);
		EnsureBuffer(TBuffer::m_BufferSize);
		return m_Buffer;
	}

protected:

	// Make sure we have a buffer we can write size bytes without stall
	void	EnsureBuffer(size_t size)
	{
		if (m_Buffer && (m_Buffer->GetSize() < (size) || BufferUpdateCausesStallGLES(m_Buffer)))
		{
			// Buffer too small or would cause stall, release and recreate
			m_Buffer->Release();
			m_Buffer = 0;
		}

		// Allocate buffer if needed
		if (!m_Buffer)
			m_Buffer = GetBufferManagerGLES()->AcquireBuffer(size, m_Usage);

		// Special case for TF targets, we'll have to actually also make room for the destination, and not just rely on lazy init
		if (m_Usage == DataBufferGLES::kTFDestination)
			m_Buffer->EnsureStorage(size);
	}


	DataBufferGLES *			m_Buffer;
	DataBufferGLES::BufferUsage m_Usage;

	dynamic_array<UInt8, AlignOfType<UInt32>::align >	m_BufferData; // Aligned to UInt32 to prevent problems with writing floats
	bool						m_IsMapped; // if true, we're inside begin/endwrite and we're using glMapBuffer
	size_t						m_WriteOffset;

};

typedef DrawBufferGLES<VertexBuffer, VertexBufferUsageMapperGLES> VertexBufferGLES;
typedef DrawBufferGLES<IndexBuffer, IndexBufferUsageMapperGLES>	IndexBufferGLES;
