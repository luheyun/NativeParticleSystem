#ifndef THREADED_STREAM_BUFFER_H
#define THREADED_STREAM_BUFFER_H

#if SUPPORT_THREADS

#include "Runtime/Utilities/NonCopyable.h"
#include "Runtime/Utilities/Utility.h"
#include <new> // for placement new
#include "Runtime/Threads/Thread.h"
#include "Runtime/Allocator/MemoryMacros.h"

// Write the size with every element so we can check that the writer and reader agree.
// If the written and read sizes don't match then they will not wrap around correctly when reaching the end of the buffer.
// Check that streambuffer is exclusively used from one thread, since it is only 1 Producer : 1 Consumer thread safe
//#define DEBUG_CHECK_THREADED_STREAM_BUFFER_USAGE (!UNITY_RELEASE)

// Overwrite released data with garbage to catch any of it still being used.
//#define DEBUG_WIPE_THREADED_BUFFER_RELEASED_DATA (!UNITY_RELEASE)

class Semaphore;

/// A single producer, single consumer ringbuffer

/// Most read and write operations are done even without atomic operations and use no expensive synchronization primitives
/// Each thread owns a part of the buffer and only locks when reaching the end


/// *** Common usage ***
/// * Create the ring buffer
/// ThreadedStreamBuffer buffer (kModeThreaded);

/// * Producer thread...
/// buffer.WriteValueType<int>(5);
/// buffer.WriteValueType<int>(7);
/// buffer.WriteSubmitData();

/// * ConsumerThread...
/// print(buffer.ReadValueType<int>());
/// print(buffer.ReadValueType<int>());
/// buffer.ReadReleaseData();

class ThreadedStreamBuffer : public NonCopyable
{
public:

	enum Mode
	{
		// This is the most common usage. One producer, one consumer on different threads.
		kModeThreaded,

		// When in read mode, we pass a pointer to the external data which can then be read using ReadValueType and ReadReleaseData.
		kModeReadOnly,

		// When in growable you are only allowed to write into the ring buffer. Essentially like a std::vector. It will keep on growing as you write data.
		kModeGrowable,
		kModeCrossProcess,
	};

	ThreadedStreamBuffer(Mode mode, UInt32 size);
	ThreadedStreamBuffer();
	~ThreadedStreamBuffer();

	enum
	{
		kDefaultAlignment = 4,
		kDefaultStep = 4096
	};
	
	// Read data from the ringbuffer
	// This function blocks until data new data has arrived in the ringbuffer.
	// It uses semaphores to wait on the producer thread in an efficient way.
	template <class T> const T&	ReadValueType();
	template <class T> T*		ReadArrayType(int count);
	// ReadReleaseData should be called when the data has been read & used completely.
	// At this point the memory will become available to the producer to write into it again.
	void						ReadReleaseData();
	
	// Write data into the ringbuffer
	template <class T> void		WriteValueType(const T& val);
	template <class T> void		WriteArrayType(const T* vals, int count);
	template <class T> T*		GetWritePointer();
	// WriteSubmitData should be called after data has been completely written and should be made available to the consumer thread to read it.
	// Before WriteSubmitData is called, any data written with WriteValueType can not be read by the consumer.
	void						WriteSubmitData();

	// Ringbuffer Streaming support. This will automatically call WriteSubmitData & ReadReleaseData.
	// It splits the data into smaller chunks (step). So that the size of the ringbuffer can be smaller than the data size passed into this function.
	// The consumer thread will be reading the streaming data while WriteStreamingData is still called on the producer thread.
	void						ReadStreamingData(void* data, UInt32 size, UInt32 alignment = kDefaultAlignment, UInt32 step = kDefaultStep);
	void						WriteStreamingData(const void* data, UInt32 size, UInt32 alignment = kDefaultAlignment, UInt32 step = kDefaultStep);

	// WakeConsumerThread will wake up the consumer thread without submitting any extra data to it.
	// This function could be called from other threads beside the consumer and producer.
	void WakeConsumerThread() { SendWriteSignal(); }
	
	// Utility functions

	void*	GetReadDataPointer(UInt32 size, UInt32 alignment);
	void*	GetWriteDataPointer(UInt32 size, UInt32 alignment);

	// Only applies to kModeGrowable buffer; queries the next write position as if GetWriteDataPointer(anySize, alignment) is called.
	UInt32	GetNextWritePosition(UInt32 alignment, UInt32 writePos = 0xffffffff) const;

	UInt32	GetDebugReadPosition() const { return m_Reader.bufferPos; }
	UInt32	GetDebugWritePosition() const { return m_Writer.bufferPos; }

	UInt32	GetMaxNonStreamSize() const;
	
	// Creation methods
	void	Create(Mode mode, UInt32 size);
	void	CreateFromMemory(Mode mode, UInt32 size, void *buffer);
	void	CreateReadOnly(const void* buffer, UInt32 size);
	void	ResetGrowable();
	void	Destroy();
	
	// Is there data available to read (typically this is not used)
	bool	HasDataToRead() const;
	
	UInt32	GetAllocatedSize() const { return m_BufferSize; }
	UInt32	GetCurrentSize() const;
	const void*	GetBuffer() const;
	
	
	////@TODO: Remove this
	typedef void (*DataConsumer)(const void* buffer, UInt32 bufferSize, void* userData);
	UInt32 ReadStreamingData(DataConsumer consumer, void* userData, UInt32 alignment = kDefaultAlignment, UInt32 step = kDefaultStep);
	
	typedef bool (*DataProvider)(void* dest, UInt32 bufferSize, UInt32& bytesWritten, void* userData);
	void   WriteStreamingData(DataProvider provider, void* userData, UInt32 alignment = kDefaultAlignment, UInt32 step = kDefaultStep);
	
	typedef void (*WaitCallback)(bool isWaiting);
	void SetReadWaitCallback (WaitCallback callback) { m_ReadWaitCallback = callback; }
	void SetWriteWaitCallback (WaitCallback callback) { m_WriteWaitCallback = callback; }
	
	typedef bool (*IdleCallback)();
	void SetIdleCallback (IdleCallback callback) { m_IdleCallback = callback; }


	void SetCurrentThreadAsProducer() { }
	void SetCurrentThreadAsConsumer() { }
	void CopyConsumer(ThreadedStreamBuffer* buffer) { }

private:
	FORCE_INLINE UInt32 Align(UInt32 val, UInt32 alignment) const { return (val + alignment - 1) & ~(alignment - 1); }

	// Align size to write/read. Always align to at least kDefaultAlignment, so position is always aligned that much.
	FORCE_INLINE UInt32 AlignSize(UInt32 size, UInt32 alignment) const { return Align(size, alignment > kDefaultAlignment ? alignment : kDefaultAlignment); }

	// Align position to write to/read from. We assume pos is already aligned to kDefaultAlignment.
	FORCE_INLINE UInt32 AlignPos(UInt32 pos, UInt32 alignment) const
	{
		return (alignment > kDefaultAlignment) ? Align(pos, alignment) : pos;
	}

	void	SetDefaults();

	UInt32	ClampToBufferSize(SInt32 signedPos) const;
	void	HandleReadOverflow(UInt32& dataPos, UInt32& dataEnd);
	void	HandleWriteOverflow(UInt32& dataPos, UInt32& dataEnd);

	void	SendReadSignal();
	void	SendWriteSignal();


	struct ALIGN_TYPE(64) CacheAlignedUInt32
	{
		UInt32 val;
	};

	struct SharedState
	{
		void Reset() volatile;

		// These are written/read without locking on either thread.
		// They have to be volatile and a native integer size for atomic access.
		volatile CacheAlignedUInt32 readerPos;
		volatile CacheAlignedUInt32 writerPos;
	};

	struct ALIGN_TYPE(64) BufferState
	{
		// These don't need to be volatile since data is only used on one thread.
		// Align to a cache line to avoid false sharing.
		void Reset();
		UInt32 bufferPos;
		UInt32 bufferEnd;
		UInt32 compareBase;
	};

	Mode m_Mode;
	char* m_Buffer;
	UInt32 m_BufferSize;
	UInt32 m_GrowStepSize;
	UInt32 m_SpinCount;
	Semaphore* m_ReadSemaphore;
	Semaphore* m_WriteSemaphore;
	volatile int m_NeedsReadSignal;
	volatile int m_NeedsWriteSignal;

	// Aligned types by themselves for better packing.
	SharedState m_Shared;
	BufferState m_Reader;
	BufferState m_Writer;

	WaitCallback m_ReadWaitCallback;
	WaitCallback m_WriteWaitCallback;
	IdleCallback m_IdleCallback;
};

FORCE_INLINE void* ThreadedStreamBuffer::GetReadDataPointer(UInt32 size, UInt32 alignment)
{
	size = AlignSize(size, alignment);
	UInt32 dataPos = AlignPos(m_Reader.bufferPos, alignment);
	UInt32 dataEnd = dataPos + size;
	if (dataEnd > m_Reader.bufferEnd)
	{
		HandleReadOverflow(dataPos, dataEnd);
	}
	m_Reader.bufferPos = dataEnd;
	return &m_Buffer[dataPos];
}
FORCE_INLINE void* ThreadedStreamBuffer::GetWriteDataPointer(UInt32 size, UInt32 alignment)
{
	size = AlignSize(size, alignment);
	UInt32 dataPos = AlignPos(m_Writer.bufferPos, alignment);
	UInt32 dataEnd = dataPos + size;
	if (dataEnd > m_Writer.bufferEnd)
	{
		HandleWriteOverflow(dataPos, dataEnd);
	}
	m_Writer.bufferPos = dataEnd;
	return &m_Buffer[dataPos];
}

FORCE_INLINE UInt32 ThreadedStreamBuffer::GetNextWritePosition(UInt32 alignment, UInt32 writePos) const
{
	UInt32 dataPos = AlignPos(writePos == 0xffffffff ? m_Writer.bufferPos : writePos, alignment);
	return dataPos;
}

template <class T> FORCE_INLINE const T& ThreadedStreamBuffer::ReadValueType()
{
	// Read simple data type from queue
	const void* pdata = GetReadDataPointer(sizeof(T), ALIGN_OF(T));
	const T& src = *reinterpret_cast<const T*>(pdata);
	return src;
}

template <class T> FORCE_INLINE T* ThreadedStreamBuffer::ReadArrayType(int count)
{
	// Read array of data from queue-
	void* pdata = GetReadDataPointer(count * sizeof(T), ALIGN_OF(T));
	T* src = reinterpret_cast<T*>(pdata);
	return src;
}

template <class T> FORCE_INLINE void ThreadedStreamBuffer::WriteValueType(const T& val)
{
	// Write simple data type to queue
	void* pdata = GetWriteDataPointer(sizeof(T), ALIGN_OF(T));
	new (pdata) T(val);
}

template <class T> FORCE_INLINE void ThreadedStreamBuffer::WriteArrayType(const T* vals, int count)
{
	// Write array of data to queue
	T* pdata = (T*)GetWriteDataPointer(count * sizeof(T), ALIGN_OF(T));
	for (int i = 0; i < count; i++)
		new (&pdata[i]) T(vals[i]);
}

template <class T> FORCE_INLINE T* ThreadedStreamBuffer::GetWritePointer()
{
	// Write simple data type to queue
	void* pdata = GetWriteDataPointer(sizeof(T), ALIGN_OF(T));
	return static_cast<T*>(pdata);
}

#endif //SUPPORT_THREADS
#endif
