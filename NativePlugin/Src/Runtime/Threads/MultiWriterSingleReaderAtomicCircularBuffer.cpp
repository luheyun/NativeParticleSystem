#include "PluginPrefix.h"
#include "Runtime/Threads/MultiWriterSingleReaderAtomicCircularBuffer.h"
#include "Runtime/Threads/AtomicOps.h"
#include "Runtime/Threads/Thread.h"

// Some useful defines for clarity

// Bit tricks for speed since size is a power of 2.
#define ACB_IsPowerOfTwo(X) ( ( (X) & ( (X)-1 ) ) == 0 )
#define ACB_ModSize(X)      ( (X) & (m_Size - 1) )
#define ACB_NextMultipleOfHeaderSize(X) (((X) + (HEADERSIZE-1)) & (~(HEADERSIZE-1)))

#if DEBUG
#define ACB_ValidateTailMarker() Assert( m_BufferEnd[0] == 'E' && m_BufferEnd[1] == 'N' && m_BufferEnd[2] == 'D' && m_BufferEnd[3] == 'M' )
#else
#define ACB_ValidateTailMarker()
#endif

#define HEADERSIZE sizeof(int)

// ============================================================
// Convenience class:
// A reserved chunk of space in the MWSRACircularBuffer
// This represents both the header and the payload in
// the buffer. You should not write to this thing.
class AtomicCircularBufferHandle
{
public:
	AtomicCircularBufferHandle( int size )
		: m_PayloadSize(size)
	{}

	int GetSize()
	{ return m_PayloadSize; }

	void SetSize(int size)
	{ m_PayloadSize = size; }

	unsigned char * GetPayload()
	{ return m_Data; }

private:
	friend class MultiWriterSingleReaderAtomicCircularBuffer;

	int           m_PayloadSize;
	unsigned char m_Data[1];
};


MultiWriterSingleReaderAtomicCircularBuffer::MultiWriterSingleReaderAtomicCircularBuffer(int size)
	: m_Buffer(0)
	, m_BufferEnd(0)
	, m_Size(size)
	, m_ReadHead(0)
	, m_ReadTail(0)
	, m_WriteHead(0)
{
	// Being a multiple of 2 ensures that we can avoid nasty mods and ensure we are always a multiple
	// of HEADERSIZE
	//Assert(size > 0 && ACB_IsPowerOfTwo(size) );

	m_Buffer    = (unsigned char*)UNITY_MALLOC(kMemThread, size);
	m_BufferEnd = m_Buffer + m_Size;
}

MultiWriterSingleReaderAtomicCircularBuffer::~MultiWriterSingleReaderAtomicCircularBuffer()
{
	UNITY_FREE(kMemThread, m_Buffer);
}

// This moves the write head or free pointer, which is not the same as the read tail.
// the data is not yet ready to consume by the reader, space is just known
// to be available to write into.
AtomicCircularBufferHandle * MultiWriterSingleReaderAtomicCircularBuffer::ReserveSpaceForData(int realSize)
{
	int F   = 0;
	int WNF = 0;

	ACB_ValidateTailMarker();

	// To ensure we always have space for a header at the end our packet sizes are always bumped up
	// to the next multiple of a header size.
	int size    = ACB_NextMultipleOfHeaderSize(realSize);
	do
	{
		F       = m_WriteHead;           // F
		int NF  = F + size + HEADERSIZE; // NF
		WNF     = ACB_ModSize( NF );     // WNF
		int H   = m_ReadHead;


		// Case 1: Zero space to start heads are the same
		//         Fail by default
		//
		// ----FH------
		// H----------F

		// Case 2: Wrapped Free pointer F < H
		//         Free cannot wrap and chases head
		// -----F-------H-----
		//

		// Case 3: Standard head / tail pointer F > H
		//         Free can wrap so two cases here:
		// Case 3a: NF == WNF cannot fail
		// -----H-------F---NF--
		//
		// Case 3b: NF != WNF fail if WNF > H
		// -----H-------F-------   NF IE:
		//  WNF
		//

		// This is the starting condition, we shouldn't be getting into
		// this after the first initial start since doing so could result
		// in a false empty condition.
		// Assert( H != F );

		  //  Case 1: automatic overflow, there was no room
		if( (H-F == 1 || (H == 0 && F == (m_Size-1)) )
		  // Case 2: Fail if NF >= H
		  || ( F < H && NF >= H )
		  // Case 3a: We wrapped but exceeded the head pointer
		  || ( F >= H && NF != WNF && WNF >= H ))
		{
			return NULL;
		}

	} while( !AtomicCompareExchange( &m_WriteHead, WNF, F));

	AtomicCircularBufferHandle * handle = reinterpret_cast<AtomicCircularBufferHandle*>(m_Buffer + F);
	// We store the non multiple of header size so we can reconstruct that on copy out of the buffer.
	handle->SetSize(realSize);
	ACB_ValidateTailMarker();

	return handle;
}

void MultiWriterSingleReaderAtomicCircularBuffer::CopyDataToBuffer(AtomicCircularBufferHandle * slot,unsigned char * data, int offset, int size)
{
	int       fullSize           = slot->GetSize();
	uintptr_t amountLeftInBuffer = 0;
	// Ensure we don't overflow.
	//Assert( offset + size <= fullSize );

	// Adjust the copy buffer based on our offset
	unsigned char * copyTo = slot->GetPayload() + offset;

	// Some space before the wrap around, copy into there first.
	if( copyTo < m_BufferEnd )
	{
		// Calculate the amount of space we have left in the circular buffer
		amountLeftInBuffer = std::min(m_Size - ( (uintptr_t)copyTo - (uintptr_t)m_Buffer ), (uintptr_t)size );
		if( amountLeftInBuffer > 0 )
			memcpy(copyTo,data,amountLeftInBuffer);

		// Readjust for any overflow at the beginning of the buffer.
		copyTo = m_Buffer;
	}
	else
	{
		// We are already wrapped around, readjust based on how much our offset has made us overflow
		copyTo = m_Buffer + ((uintptr_t)m_BufferEnd - (uintptr_t)copyTo);
	}

	int amountWrapAround = size - amountLeftInBuffer;
	if( amountWrapAround > 0 )
		memcpy(copyTo,data+amountLeftInBuffer,amountWrapAround);
}

// This moves the read tail, which is the end of the data available to consume by the reader.
// This must never get ahead of the write head or read head for that matter.
void MultiWriterSingleReaderAtomicCircularBuffer::CopyDataAndMakeAvailableForRead(AtomicCircularBufferHandle * slot, unsigned char * data, int copyOffset, int copySize )
{
	ACB_ValidateTailMarker();

	// Do the work of filling out my payload
	int realSize = slot->GetSize();
	int size     = ACB_NextMultipleOfHeaderSize(realSize);

	if( data != NULL && copySize > 0)
	{
		CopyDataToBuffer(slot,data,copyOffset,copySize);
	}
	// This release of the block has to happen in sequence
	// We will spin until all other blocks are released in front
	// of us. This could result in a deadlock if you try to fill the buffer
	// from two different priority threads running on the same core
	int  T = 0;
	int  F = 0;
	bool haveWritten  = false;
	while( !haveWritten )
	{
		T        = m_ReadTail;
		F        = m_WriteHead;

		// Some other payloads before us in the buffer are getting filled out
		// because we cannot release the slot before them (for the sake of the read queue)
		// we may have to spin and wait for those threads to release up to us.
		if( (m_Buffer+T) != reinterpret_cast<unsigned char*>(slot) )
		{
			Thread::Sleep(0.000001);
			continue;
		}

		int NT  = T + size + HEADERSIZE;
		int WNT = ACB_ModSize( NT );

		haveWritten          = AtomicCompareExchange( &m_ReadTail , WNT, T );

		// Ensure we maintained validity with this update:
		//
		// Case 1: Standard T < F
		//         WNT <= F T can catch up but not pass F - we should not wrap: WNT == NT
		// ----T------F----

		// Case 2: Wrapped F: T > F
		//         WNT == NT no wrap cannot fail
		//         WNT != NT ok if T catches up to F: WNT <= F
		// ----F------T----

		//Assert( ( T < F && WNT == NT && WNT <= F )
		//	 || ( T > F && ( WNT == NT || WNT <= F )));
	}
	ACB_ValidateTailMarker();
}

// This + ReadNextPayload is not safe from multiple threads.
int MultiWriterSingleReaderAtomicCircularBuffer::GetNextPayloadSize()
{
	ACB_ValidateTailMarker();

	int H = m_ReadHead;
	int T = m_ReadTail;

	// Sanity Check: validate that we are always marching in HEADERSIZE chunks
	// around the buffer
#if DEBUG
	Assert( ACB_NextMultipleOfHeaderSize(H) == H );
	Assert( ACB_NextMultipleOfHeaderSize(T) == T );
#endif

	// Am I empty?
	if( H == T )
	{
		return 0;
	}

	// This is the bit that makes this not thread safe to read from multiple threads.
	AtomicCircularBufferHandle * current = reinterpret_cast<AtomicCircularBufferHandle*>(m_Buffer + H);
	ACB_ValidateTailMarker();
	return current->GetSize();
}

bool MultiWriterSingleReaderAtomicCircularBuffer::ReadNextPayload(unsigned char * buffer, int maxSize )
{
	int H = m_ReadHead;
	int T = m_ReadTail;

	// Validate that we are always marching in HEADERSIZE chunks
	// around the buffer
//#if DEBUG
//	Assert( ACB_NextMultipleOfHeaderSize(H) == H );
//	Assert( ACB_NextMultipleOfHeaderSize(T) == T );
//#endif

	ACB_ValidateTailMarker();

	// Case 1: H == T - empty
	if( H == T )
	{
		return false;
	}

	AtomicCircularBufferHandle * current = reinterpret_cast<AtomicCircularBufferHandle*>(m_Buffer + H);
	int realSize = current->GetSize();
	int size     = ACB_NextMultipleOfHeaderSize(realSize);

	// Clamp to avoid memory corruption even though this will probably
	// blow up later given that there is a size mismatch here.
	// We assert above to catch this as well.
	if( realSize > maxSize )
		realSize = maxSize;

	unsigned char * data = current->GetPayload();

	// Wrap around copy of data in circular buffer out into provided buffer
	int copyAmount = std::min( (uintptr_t)m_BufferEnd - (uintptr_t)data, (uintptr_t)size );
	if( copyAmount )
		memcpy(buffer, data, copyAmount);
	int wrapCopyAmount = size - copyAmount;
	if( wrapCopyAmount > 0 )
		memcpy(buffer+copyAmount,m_Buffer,wrapCopyAmount);


	// Release read head, no atomics here because we assume the read head
	// is only happening from a single thread.
	int NH  = H + size + HEADERSIZE;
	int WNH = ACB_ModSize(NH);

	// Assert against underflow, we should never see the tail and head play bad with
	// each other even when somewhat out of sync.

	// Case 2: H < T
	//         Cannot wrap WNH == NH && NewHead <= T because can be empty now
	// ----H------T----

	// Case 3: H > T
	//         Can wrap, no wrap case cannot fail WNH == NH is a pass
	//         Wrap case can fail if WNH > T IE: WNH <= T is a pass
	// ----T------H----

	//Assert( ( H < T && WNH == NH && NH <= T ) ||
	//		( H > T && (WNH == NH || WNH <= T )));

	m_ReadHead         = WNH;
	ACB_ValidateTailMarker();
	return true;
}

// Be good and cleanup any defines we are using
#undef ACB_IsPowerOfTwo
#undef ACB_ModSize
#undef ACB_NextMultipleOfHeaderSize
#undef ACB_ValidateTailMarker
#undef HEADERSIZE
