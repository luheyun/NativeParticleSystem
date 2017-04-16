#include "PluginPrefix.h"

#if SUPPORT_THREADS

#include "ThreadedStreamBuffer.h"
#include "Runtime/Threads/Semaphore.h"
#include "Runtime/Threads/ThreadUtility.h"
#include "Runtime/Threads/AtomicOps.h"
#include "Runtime/Threads/AtomicQueue.h"
#include "Runtime/Utilities/BitUtility.h"
#include "Runtime/Threads/ThreadToCoreMappings.h"

ThreadedStreamBuffer::ThreadedStreamBuffer()
{
	SetDefaults();
}

ThreadedStreamBuffer::ThreadedStreamBuffer(Mode mode, UInt32 size)
{
	SetDefaults();
	Create(mode, size);
}

ThreadedStreamBuffer::~ThreadedStreamBuffer()
{
	Destroy();
}

void ThreadedStreamBuffer::Create(Mode mode, UInt32 size)
{
	m_Mode = mode;
	if (size != 0)
		m_Buffer = (char*)UNITY_MALLOC_ALIGNED(kMemUtility, size, 64);
	m_BufferSize = size;
	m_Shared.Reset();
	m_Reader.Reset();
	m_Writer.Reset();
	m_Writer.bufferEnd = size;
	
	SetCurrentThreadAsProducer();
	SetCurrentThreadAsConsumer();

	if (m_Mode == kModeThreaded)
	{
		m_ReadSemaphore = new Semaphore;
		m_WriteSemaphore = new Semaphore;
	}
}

void ThreadedStreamBuffer::CreateReadOnly(const void* buffer, UInt32 size)
{
	m_Mode = kModeReadOnly;
	m_Buffer = (char*)buffer;
	m_BufferSize = size;
	m_Shared.Reset();
	m_Reader.Reset();
	m_Writer.Reset();
	m_Reader.bufferEnd = size;
	m_Shared.writerPos.val = size;
	SetCurrentThreadAsConsumer();
}

void ThreadedStreamBuffer::ResetGrowable()
{
	m_Reader.Reset();
	m_Writer.Reset();
	m_Writer.bufferEnd = m_BufferSize;
}

void ThreadedStreamBuffer::Destroy()
{
	if (m_Buffer == NULL) return;
	if (m_Mode != kModeReadOnly)
	UNITY_FREE(kMemUtility, m_Buffer);
	m_Reader.Reset();
	m_Writer.Reset();
	if (m_Mode == kModeThreaded)
	{
		delete m_ReadSemaphore;
		delete m_WriteSemaphore;
	}
	SetDefaults();
}

UInt32 ThreadedStreamBuffer::GetCurrentSize() const
{
	if (m_Mode == kModeGrowable)
	{
		return m_Writer.bufferPos;
	}
	return m_BufferSize;
}

const void*	ThreadedStreamBuffer::GetBuffer() const
{
	//Assert(m_Mode != kModeThreaded);
	return m_Buffer;
}

void ThreadedStreamBuffer::ReadStreamingData(void* data, UInt32 size, UInt32 alignment, UInt32 step)
{
	//Assert((step % alignment) == 0);

	UInt32 sz = ReadValueType<UInt32>();

	char* dest = (char*)data;
	for (UInt32 offset = 0; offset < size; offset += step)
	{
		UInt32 bytes = std::min(size - offset, step);
		const void* src = GetReadDataPointer(bytes, alignment);
		if (data)
			UNITY_MEMCPY(dest, src, bytes);
		int magic = ReadValueType<int>();
		//Assert(magic == 1234);
		ReadReleaseData();
		dest += step;
	}
}

UInt32 ThreadedStreamBuffer::ReadStreamingData(DataConsumer consumer, void* userData, UInt32 alignment, UInt32 step)
{
	//Assert((step % alignment) == 0);
	//Assert(consumer != NULL);

	UInt32 totalBytesRead = 0;

	bool moreData = false;
	do
	{
		const void* src = GetReadDataPointer(step + sizeof(UInt32), alignment);
		const UInt32 bytesInBuffer = *(static_cast<const UInt32*>(src));
		consumer(static_cast<const UInt32*>(src) + 1, bytesInBuffer, userData);
		totalBytesRead += bytesInBuffer;
		int magic = ReadValueType<int>();
		//Assert(magic == 1234);
		moreData = ReadValueType<bool>();
		ReadReleaseData();
	} while (moreData);

	return totalBytesRead;
}

void ThreadedStreamBuffer::ReadReleaseData()
{
	UnityMemoryBarrier();
	m_Shared.readerPos.val = m_Reader.compareBase + m_Reader.bufferPos;
	UnityMemoryBarrier();
	SendReadSignal();
}

void ThreadedStreamBuffer::WriteStreamingData(const void* data, UInt32 size, UInt32 alignment, UInt32 step)
{
	WriteValueType<UInt32>(size);
	//Assert((step % alignment) == 0);

	const char* src = (const char*)data;
	for (UInt32 offset = 0; offset < size; offset += step)
	{
		UInt32 bytes = std::min(size - offset, step);
		void* dest = GetWriteDataPointer(bytes, alignment);
		UNITY_MEMCPY(dest, src, bytes);
		WriteValueType<int>(1234);
		WriteSubmitData();
		src += step;
	}
	WriteSubmitData();
}

void ThreadedStreamBuffer::WriteStreamingData(DataProvider provider, void* userData, UInt32 alignment, UInt32 step)
{
	//Assert((step % alignment) == 0);
	//Assert(provider != NULL);

	bool moreData = false;
	do
	{
		void* dest = GetWriteDataPointer(step + sizeof(UInt32), alignment);
		UInt32 outBytesWritten = 0;
		moreData = provider(static_cast<UInt32*>(dest) + 1, step, outBytesWritten, userData);
		*((UInt32*)dest) = outBytesWritten;
		WriteValueType<int>(1234);
		WriteValueType(moreData);
		WriteSubmitData();
	} while (moreData);
	
	WriteSubmitData();
}

void ThreadedStreamBuffer::WriteSubmitData()
{
	UnityMemoryBarrier();
	m_Shared.writerPos.val = m_Writer.compareBase + m_Writer.bufferPos;
	UnityMemoryBarrier();
	SendWriteSignal();
}

void ThreadedStreamBuffer::SetDefaults()
{
	m_Mode = kModeReadOnly;
	m_Buffer = NULL;
	m_BufferSize = 0;
	m_GrowStepSize = 8*1024;
#if defined(GFX_THREAD_SPIN_COUNT)
	m_SpinCount = GFX_THREAD_SPIN_COUNT;
#else
	m_SpinCount = 10000;
#endif
	m_ReadSemaphore = NULL;
	m_WriteSemaphore = NULL;
	m_NeedsReadSignal = 0;
	m_NeedsWriteSignal = 0;
	m_ReadWaitCallback = NULL;
	m_WriteWaitCallback = NULL;
	m_IdleCallback = NULL;
};

bool ThreadedStreamBuffer::HasDataToRead() const
{
	// Note that this isn't necessarily equal to the number of bytes written
	// into the buffer, as data is aligned to the nearest 4-byte boundary.
	UInt32 logicalReadPos = m_Reader.compareBase + m_Reader.bufferPos;
	UnityMemoryBarrier();
	SInt32 bytesToRead = SInt32(m_Shared.writerPos.val - logicalReadPos);
	return bytesToRead > 0;
}

UInt32 ThreadedStreamBuffer::ClampToBufferSize(SInt32 signedPos) const
{
	return std::min(std::max(signedPos, SInt32(0)), SInt32(m_BufferSize));
}

void ThreadedStreamBuffer::HandleReadOverflow(UInt32& dataPos, UInt32& dataEnd )
{
	//Assert(m_Mode == kModeThreaded);

	if (dataEnd > m_BufferSize)
	{
		dataEnd -= dataPos;
		dataPos = 0;
		// Wrap around, store wrapped distance in compareBase
		m_Reader.bufferPos = 0;
		m_Reader.compareBase += m_BufferSize;
	}

	UInt32 spin = 0;
	for (;;)
	{
		// Save writer's submitted position
		UInt32 writerComparedPos = m_Shared.writerPos.val;
		// Get available bytes for reading from beginning of buffer
		SInt32 bytesAvailable = SInt32(writerComparedPos - m_Reader.compareBase);
		m_Reader.bufferEnd = ClampToBufferSize(bytesAvailable);

		if (dataEnd <= m_Reader.bufferEnd)
		{
			// Buffer space available!
			break;
		}

		if (m_IdleCallback)
		{
			if (m_IdleCallback ())
			{
				spin = 0;
				continue;
			}
			else
			{
				AtomicList::Relax();
			}
		}

		if (spin < m_SpinCount)
		{
			// Spin before going idle
			++spin;
			continue;
		}

		AtomicIncrement(&m_NeedsWriteSignal);
		UnityMemoryBarrier();
		if (writerComparedPos != m_Shared.writerPos.val)
		{
			// Writer position changed while we requested a signal
			// Request might be missed, so we signal ourselves to avoid deadlock
			SendWriteSignal();
		}

		SendReadSignal();
		
		if (m_ReadWaitCallback)
			m_ReadWaitCallback (true);

		// Wait for writer thread
		m_WriteSemaphore->WaitForSignal();

		if (m_ReadWaitCallback)
			m_ReadWaitCallback (false);
	}
}

void ThreadedStreamBuffer::HandleWriteOverflow(UInt32& dataPos, UInt32& dataEnd)
{
	if (m_Mode == kModeGrowable)
	{
		UInt32 dataSize = dataEnd - dataPos;
		UInt32 growSize = std::max(dataSize, m_GrowStepSize);
		m_BufferSize += growSize;
		m_Buffer = (char*)UNITY_REALLOC_(kMemUtility, m_Buffer, m_BufferSize);
		m_Writer.bufferEnd = m_BufferSize;
		return;
	}

	if (dataEnd > m_BufferSize)
	{
		dataEnd -= dataPos;
		dataPos = 0;
		// Wrap around, store wrapped distance in compareBase
		m_Writer.bufferPos = 0;
		m_Writer.compareBase += m_BufferSize;
	}

	UInt32 spin = 0;
	for (;;)
	{
		// Save reader's released position
		UInt32 readerComparedPos = m_Shared.readerPos.val;
		// Get available bytes for writing from beginning of buffer
		SInt32 bytesAvailable = SInt32(m_BufferSize + readerComparedPos - m_Writer.compareBase);
		m_Writer.bufferEnd = ClampToBufferSize(bytesAvailable);

		if (dataEnd <= m_Writer.bufferEnd)
		{
			// Buffer space available!
			break;
		}
		if (spin < m_SpinCount)
		{
			// Spin before going idle
			++spin;
			continue;
		}

		AtomicIncrement(&m_NeedsReadSignal);
		UnityMemoryBarrier();
		if (readerComparedPos != m_Shared.readerPos.val)
		{
			// Reader position changed while we requested a signal
			// Request might be missed, so we signal ourselves to avoid deadlock
			SendReadSignal();
		}
		SendWriteSignal();
		
		if (m_WriteWaitCallback)
			m_WriteWaitCallback (true);

		// Wait for reader thread
		m_ReadSemaphore->WaitForSignal();

		if (m_WriteWaitCallback)
			m_WriteWaitCallback (false);
	}
}

void ThreadedStreamBuffer::SendReadSignal()
{
	if (AtomicCompareExchange(&m_NeedsReadSignal, 0, 1) )
	{
		m_ReadSemaphore->Signal();
	}
}

void ThreadedStreamBuffer::SendWriteSignal()
{
	if (AtomicCompareExchange(&m_NeedsWriteSignal, 0, 1) )
	{
		m_WriteSemaphore->Signal();
	}
}

UInt32 ThreadedStreamBuffer::GetMaxNonStreamSize() const
{
	return m_BufferSize / 2;
}

void ThreadedStreamBuffer::SharedState::Reset() volatile
{
	readerPos.val = 0;
	writerPos.val = 0;
}

void ThreadedStreamBuffer::BufferState::Reset()
{
	bufferPos = 0;
	bufferEnd = 0;
	compareBase = 0;
}

#endif //SUPPORT_THREADS
