#pragma once

class AtomicCircularBufferHandle;

// ============================================================
// A simple atomic circular buffer with minimal fault tolerance
// assumes one reader multiple writers. This thing does not
// grow and it will simply fail if there is not room in the
// buffer. All checking must be done externally.
class MultiWriterSingleReaderAtomicCircularBuffer
{
public:
	// Size must be a multiple of 2, validated in debug builds.
	explicit MultiWriterSingleReaderAtomicCircularBuffer( int size );
	~MultiWriterSingleReaderAtomicCircularBuffer();

	// Write, can be done from multiple threads
	AtomicCircularBufferHandle * ReserveSpaceForData(int size);
	void CopyDataToBuffer(AtomicCircularBufferHandle * slot,unsigned char * data, int offset, int size);
	void CopyDataAndMakeAvailableForRead(AtomicCircularBufferHandle * slot,unsigned char * data = NULL, int offset = 0, int size = 0 );

	// Reading, should only be done from one thread.
	// If the payload size is always known to be a max size
	// it can be done from multiple threads but you must validate
	// this.
	bool ReadNextPayload( unsigned char * buffer , int maxSize );
	int  GetNextPayloadSize();

protected:
	unsigned char * m_Buffer;
	unsigned char * m_BufferEnd;
	int             m_Size;
	volatile int    m_ReadHead;
	volatile int    m_ReadTail;  // This is also known as the write head
	volatile int    m_WriteHead; // This is currently reserved space we are writing into.
};
