#pragma once

#include "Configuration/UnityConfigure.h"
#include "Runtime/GfxDevice/GfxDeviceConfigure.h"

//#include "Runtime/GfxDevice/GfxDeviceObjects.h"
#include "Runtime/GfxDevice/GfxDeviceTypes.h"

#include "ApiFuncGLES.h"
#include "ApiEnumGLES.h"

class BufferManagerGLES;

enum
{
	kBufferUpdateMinAgeGLES		= 4 //!< This many frames must be elapsed since last render before next buffer update. \todo [2013-05-31 pyry] From GfxDevice caps
};

class DataBufferGLES
{
public:
	typedef enum
	{
		kStaticVBO = 0,		// Static vertex data, write-once, draw-many
		kDynamicVBO,		// Dynamic vertex data, update often from CPU
		kCircularVBO,		// Circular vertex data, write-while-rendering.
		kTFDestination,		// Dynamic vertex data, written to by transform feedback.
		kStaticIBO,			// Static index buffer object
		kDynamicIBO,		// Dynamic index buffer object
		kCircularIBO,		// Circular index data, write-while-rendering
		kStaticUBO,			// Static uniform buffer object
		kDynamicUBO,		// Dynamic uniform buffer object
		kDynamicSSBO,		// Shader Storage Buffer object, always dynamic
		kDynamicACBO,		// Atomic counter buffer object, TODO: always dynamic?
		kBufferUsageCount	// Must be last item in list
	} BufferUsage;

	// When <clear> is set to true, the buffer is initialized with zeros
							DataBufferGLES			(BufferManagerGLES& bufferManager, int size, BufferUsage usage, bool clear = false);
							~DataBufferGLES			();

	void					Release					(void); //!< Release to BufferManager.

	UInt32					GetBuffer				(void) const { return m_buffer; }
	int						GetSize					(void) const { return m_size;	}
	BufferUsage				GetUsage				(void) const { return m_usage;	}


	void					RecreateStorage			(int size);
	void					EnsureStorage			(int size); // Like RecreateStorage, but doesn't do anything if buffer storage is already allocated
	void					RecreateWithData		(int size, const void* data);
	void					Upload					(int offset, int size, const void* data);
	void					Clear					();

	void*					Map						(int offset, int size, UInt32 mapBits);
	void					FlushMappedRange		(int offset, int size);
	void					Unmap					(void);

	void					CopySubData				(const DataBufferGLES* src, int srcOffset, int dstOffset, int size);

	void					RecordRecreate			(int size);					//!< Updates storage parameters and recreate time.
	void					RecordUpdate			(void);						//!< Updates last update time if buffer was updated manually.
	void					RecordRender			(void);						//!< Updates last render time.

	UInt32					GetRecreateAge			(void) const;
	UInt32					GetUpdateAge			(void) const;
	UInt32					GetRenderAge			(void) const;
	bool					UpdateCausesStall		(void) const;

	//! Disown and remove buffer handle. Used if destructor should not try to delete buffer..
	void					Disown					(void);

private:
							DataBufferGLES			(const DataBufferGLES& other);
	DataBufferGLES&			operator=				(const DataBufferGLES& other);

	static GLenum			mapBufferUsageToGLUsage	(BufferUsage usage);

	BufferManagerGLES&	m_manager;
	UInt32					m_buffer;
	int						m_size;
	const BufferUsage		m_usage;
	const GLenum			m_glUsage;
	bool					m_storageAllocated;		//!< true if glBufferData has actually been called for this buffer.
	bool					m_mappedForReadOnly;	// If true, the previous map was for reading only, so no point in doing RecordUpdate() in unmap

	// \note Always used to compute relative age and overflow is handled
	//		 in computation. Thus frame index can safely overflow.
	UInt32					m_lastRecreated;		//!< Last recreated.
	UInt32					m_lastUpdated;			//!< Frame index when last updated.
	UInt32					m_lastRendered;			//!< Frame index when last rendered.
};

// BufferManager
//
// BufferManager is responsible for allocating and maintaining list of free buffer objects that
// could be recycled later on. Buffers are either allocated or recycled based on their properties.
// Most important property for proper use of buffers is to make sure they are not recycled
// too soon after using them for rendering.
//
// BufferManager is only responsible for managing currently free buffers. So user must either
// release or destroy buffer objects manually. User is also responsible of implementing sane
// usage patterns for buffers that it owns (for example not updating data right after buffer
// has been submitted for rendering).
//
// Buffers are associated to the BufferManager that was used to create them. Thus user must either
// destroy buffer, or release it back to same BufferManager.
//
// The best usage pattern for leveraging BufferManager is to always release buffers when there
// is no longer need to preserve the data in buffer object. That way BufferManager takes care
// of recycling buffer when it is appropriate.

class BufferManagerGLES
{
public:
							BufferManagerGLES		();
							~BufferManagerGLES		();

	//! Acquire a new or recycled buffer. Returns buffer object that can fit *size* bytes of data.
	DataBufferGLES*			AcquireBuffer			(int size, DataBufferGLES::BufferUsage usage, bool clear = false);
	void					ReleaseBuffer			(DataBufferGLES* buffer);

	void					AdvanceFrame			(); //!< Advance frame index. Must be called at the end of frame.
	UInt32					GetFrameIndex			() const { return m_frameIndex; }
	UInt32					GetLastCompletedFrameIndex() const { return m_LastCompletedFrameIndex; }

	//!< Invalidate all owned buffers. Used on context loss.
	void					InvalidateAll			();

private:
							BufferManagerGLES		(const BufferManagerGLES& other);
	BufferManagerGLES&		operator=				(const BufferManagerGLES& other);

	void					Clear					();

	void					PruneFreeBuffers		();
	UInt32					GetTotalFreeSize		();

	void					UpdateLiveSetFromPending();
	void					InsertIntoLive			(DataBufferGLES* buffer);

	UInt32					m_frameIndex;	//!< Frame index for computing buffer ages.

	// Buffers that can not be selected are in pendingBuffers. Live buffers contain
	// buffers organized by each usage.
	std::vector<DataBufferGLES*>		m_pendingBuffers;
	// Multimap of live databuffers, key is the size of each live buffer.
	std::multimap<int, DataBufferGLES*>	m_liveBuffers[DataBufferGLES::kBufferUsageCount];

	struct SyncFence
	{
		GLsync m_Fence;
		UInt32 m_FrameIndex;
	};

	std::list<SyncFence>	m_LiveFences;				// List of GL fences that we haven't reached yet.
	UInt32					m_LastCompletedFrameIndex;	// The highest frame index for which we have reached the fence sync, or m_FrameIndex - kBufferUpdateMinAgeGLES if fences not available

};

// Get or create an global buffer manager
BufferManagerGLES*	GetBufferManagerGLES();

// Delete the global buffer manager
void ReleaseBufferManagerGLES();

//! Determine if buffer update will likely cause GPU stall.
bool BufferUpdateCausesStallGLES(const DataBufferGLES* buffer);

// We must call for every rendered frame this function to make the buffer recycle system move forward otherwise we leak.
void GfxDeviceAdvanceFrameGLES();
