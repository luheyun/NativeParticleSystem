#include "PluginPrefix.h"

#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "DataBuffersGLES.h"
#include "AssertGLES.h"
#include "ApiGLES.h"
#include "Runtime/GfxDevice/GfxDevice.h"
#include "Runtime/Shaders/GraphicsCaps.h"

#include <algorithm>

#if 1
	#define DBG_LOG_BUF_GLES(...) {}
#else
	#define DBG_LOG_BUF_GLES(...) {printf_console(__VA_ARGS__);printf_console("\n");}
#endif

#if UNITY_64
#	define DATA_BUFFER_ID_MASK 0xC000000000000000
#	define MAKE_DATA_BUFFER_ID(id) (UInt64)(id|DATA_BUFFER_ID_MASK)
#else
#	define DATA_BUFFER_ID_MASK 0xC0000000
#	define MAKE_DATA_BUFFER_ID(id) (id|DATA_BUFFER_ID_MASK)
#endif // _LP64

static const UInt32	kBufferPruneFrequency			= 10;				//!< Unneeded buffers are pruned once per kBufferPruneFrequency frames.

bool BufferUpdateCausesStallGLES (const DataBufferGLES* buffer)
{
	return buffer->UpdateCausesStall();
}

gl::BufferTarget translateToBufferTarget(DataBufferGLES::BufferUsage usage)
{
	switch (usage)
	{
	case DataBufferGLES::kStaticVBO:
	case DataBufferGLES::kDynamicVBO:
	case DataBufferGLES::kCircularVBO:
		return (GetGraphicsCaps().gles.useActualBufferTargetForUploads || !GetGraphicsCaps().gles.hasBufferCopy) ? gl::kArrayBuffer : gl::kCopyWriteBuffer;
	case DataBufferGLES::kStaticIBO:
	case DataBufferGLES::kDynamicIBO:
	case DataBufferGLES::kCircularIBO:
		return (GetGraphicsCaps().gles.useActualBufferTargetForUploads || !GetGraphicsCaps().gles.hasBufferCopy) ? gl::kElementArrayBuffer : gl::kCopyWriteBuffer;
	case DataBufferGLES::kStaticUBO:
	case DataBufferGLES::kDynamicUBO:
	case DataBufferGLES::kTFDestination:
	case DataBufferGLES::kDynamicSSBO:
	case DataBufferGLES::kDynamicACBO:
		return !GetGraphicsCaps().gles.hasBufferCopy ? gl::kArrayBuffer : gl::kCopyWriteBuffer;
	default:
		return gl::kBufferTargetInvalid;
	}
}

GLenum DataBufferGLES::mapBufferUsageToGLUsage(BufferUsage usage)
{
	switch (usage)
	{
	case kStaticVBO:
	case kStaticIBO:
	case kStaticUBO:
		return GL_STATIC_DRAW;
	case kDynamicVBO:
	case kDynamicIBO:
	case kDynamicUBO:
	case kDynamicSSBO:
	case kDynamicACBO:
		return GL_DYNAMIC_DRAW;
	case kCircularIBO:
	case kCircularVBO:
		return GL_STREAM_DRAW;
	case kTFDestination:
		return GL_STATIC_COPY; // TODO: figure out if this is actually correct.
	default:
		return GL_DYNAMIC_DRAW;
	}
}

// DataBufferGLES

DataBufferGLES::DataBufferGLES(BufferManagerGLES& bufferManager, int size, BufferUsage usage, bool clear)
	: m_manager			(bufferManager)
	, m_buffer			(0)
	, m_size			(size)
	, m_usage			(usage)
	, m_glUsage			(mapBufferUsageToGLUsage(usage))
	, m_storageAllocated(false)
	, m_lastRecreated	(0)
	, m_lastUpdated		(0)
	, m_lastRendered	(0)
{
	if (clear)
	{
		this->RecreateStorage(size);
		if (GetGraphicsCaps().gles.hasBufferClear)
			gGL->ClearBuffer(m_buffer, translateToBufferTarget(m_usage));
		else
			gGL->ClearBufferSubData(m_buffer, translateToBufferTarget(m_usage), 0, size);
	}

	DBG_LOG_BUF_GLES("DataBuf ctor: this %08X, m_buffer: %d\n", (int)this, m_buffer);
}

DataBufferGLES::~DataBufferGLES()
{
	DBG_LOG_BUF_GLES("DataBuf dtor: this %08X, m_buffer: %d\n", (int)this, m_buffer);

	if (m_storageAllocated)
		REGISTER_EXTERNAL_GFX_DEALLOCATION(MAKE_DATA_BUFFER_ID(m_buffer));

	if (m_buffer)
		gGL->DeleteBuffer(m_buffer);
}

void DataBufferGLES::Disown()
{
	DBG_LOG_BUF_GLES("DataBuf::Disown: this %08X, m_buffer: %d\n", (int)this, m_buffer);
	m_buffer = 0;
}

void DataBufferGLES::Release()
{
	DBG_LOG_BUF_GLES("DataBuf release: this %08X, m_buffer: %d\n", (int)this, m_buffer);
	m_manager.ReleaseBuffer(this);
}

void DataBufferGLES::RecreateStorage(int size)
{
	RecreateWithData(size, 0);
}

void DataBufferGLES::EnsureStorage(int size)
{
	if (!m_storageAllocated || GetSize() < size)
		RecreateStorage(size);
}

void DataBufferGLES::RecreateWithData(int size, const void* data)
{
	if (!m_buffer)
		m_buffer = gGL->CreateBuffer(translateToBufferTarget(m_usage), size, data, m_glUsage);
	else
		m_buffer = gGL->RecreateBuffer(m_buffer, translateToBufferTarget(m_usage), size, data, m_glUsage);

	DBG_LOG_BUF_GLES("DataBuf::RecreateWithData: this: %08X buf: %d size: %d dataptr: %08X\n", (int)this, m_buffer, size, (int)data);

	RecordRecreate(size);
}

void DataBufferGLES::Upload(int offset, int size, const void* data)
{
	DBG_LOG_BUF_GLES("DataBuf upload: this %08X, m_buffer: %d\n", (int)this, m_buffer);
	if (!m_storageAllocated)
	{
		if (offset == 0 && size == m_size)
		{
			RecreateWithData(size, data);
			return;
		}
		else
			RecreateStorage(m_size);
	}
	if (data)
	{
		gGL->UploadBufferSubData(m_buffer, translateToBufferTarget(m_usage), offset, size, data);
	}

	RecordUpdate();
}

void* DataBufferGLES::Map(int offset, int size, UInt32 mapBits)
{
	if (!m_storageAllocated)
		RecreateStorage(m_size);

	void* ptr = gGL->MapBuffer(m_buffer, translateToBufferTarget(m_usage), offset, size, mapBits);

	m_mappedForReadOnly = (mapBits & GL_MAP_READ_BIT) && !(mapBits & GL_MAP_WRITE_BIT);

	return ptr;
}

void DataBufferGLES::FlushMappedRange(int offset, int size)
{
	if (!GetGraphicsCaps().gles.hasMapbufferRange)
		return;

	gGL->FlushBuffer(m_buffer, translateToBufferTarget(m_usage), offset, size);
}

void DataBufferGLES::Unmap()
{
	gGL->UnmapBuffer(m_buffer, translateToBufferTarget(m_usage));

	if (!m_mappedForReadOnly)
		RecordUpdate();
}

void DataBufferGLES::CopySubData(const DataBufferGLES* src, int srcOffset, int dstOffset, int size)
{
	EnsureStorage(m_size);

	gGL->CopyBufferSubData(src->GetBuffer(), m_buffer, srcOffset, dstOffset, size);
	
	RecordUpdate();
}

void DataBufferGLES::RecordRecreate(int size)
{
	if (BufferUpdateCausesStallGLES(this))
		DBG_LOG_BUF_GLES("DataBufferGLES: Warning: buffer with render age %u was recreated!", GetRenderAge());

	if (m_storageAllocated)
		REGISTER_EXTERNAL_GFX_DEALLOCATION(MAKE_DATA_BUFFER_ID(m_buffer));

	m_size			= size;
	m_lastRecreated	= m_manager.GetFrameIndex();
	m_storageAllocated = true;

	// Update GFX mem allocation stats
	REGISTER_EXTERNAL_GFX_ALLOCATION_REF(MAKE_DATA_BUFFER_ID(m_buffer), size, this);
}

void DataBufferGLES::RecordUpdate()
{
	if (BufferUpdateCausesStallGLES(this))
		DBG_LOG_BUF_GLES("DataBufferGLES: Warning: buffer with render age %u was updated!", GetRenderAge());

	m_lastUpdated = m_manager.GetFrameIndex();
}

void DataBufferGLES::RecordRender()
{
	m_lastRendered = m_manager.GetFrameIndex();
}

// \note Overflow is perfectly ok here.

UInt32 DataBufferGLES::GetRecreateAge() const
{
	return m_manager.GetFrameIndex() - m_lastRecreated;
}

UInt32 DataBufferGLES::GetUpdateAge() const
{
	return m_manager.GetFrameIndex() - m_lastUpdated;
}

UInt32 DataBufferGLES::GetRenderAge() const
{
	return m_manager.GetFrameIndex() - m_lastRendered;
}

bool DataBufferGLES::UpdateCausesStall() const
{
	// \note If needed, the min age should me made ES3 specific graphics capability.
	// Note that on the editor this might well be quite wrong as there are multiple contexts and the concept of "frame" is quite ambiguous
	// So we tick the manager at every present (= once per each rendered window/tab)
	// Luckily all editor environments have sync fences so the fixed frame count only gets used on ES2 targets.

	// Handle 32-bit counter overflow, just return OK to update
	if (m_lastRendered > m_manager.GetFrameIndex())
		return false;

	return m_manager.GetLastCompletedFrameIndex() < m_lastRendered;
}

// BufferManagerGLES

BufferManagerGLES::BufferManagerGLES()
	: m_frameIndex(kBufferUpdateMinAgeGLES) // Start at an offset so the last completed frame can start from 0
	, m_LastCompletedFrameIndex(0)
{
}

BufferManagerGLES::~BufferManagerGLES()
{
	Clear();
}

void BufferManagerGLES::Clear()
{
	for (std::vector<DataBufferGLES*>::iterator i = m_pendingBuffers.begin(); i != m_pendingBuffers.end(); i++)
		UNITY_DELETE(*i, kMemGfxDevice);
	m_pendingBuffers.clear();

	for (int ndx = 0; ndx < DataBufferGLES::kBufferUsageCount; ndx++)
	{
		for (std::multimap<int, DataBufferGLES*>::iterator i = m_liveBuffers[ndx].begin(); i != m_liveBuffers[ndx].end(); i++)
			UNITY_DELETE((*i).second, kMemGfxDevice);
		m_liveBuffers[ndx].clear();
	}
}


DataBufferGLES* BufferManagerGLES::AcquireBuffer(int size, DataBufferGLES::BufferUsage usage, bool clear)
{
    // The logic here:
    // First try to get an exact match for the size, then take the next available one unless it's over 1.5x the size,
    // in which case allocate a new one.
    std::multimap<int, DataBufferGLES*>::iterator itr = m_liveBuffers[usage].lower_bound(size);

    if ((itr != m_liveBuffers[usage].end()) && ((*itr).first < (int)(3 * size / 2)))
    {
        // Match found, remove from live buffers and return
        DataBufferGLES *selected = (*itr).second;
        m_liveBuffers[usage].erase(itr);

        if (clear)
            gGL->ClearBufferSubData(selected->GetBuffer(), translateToBufferTarget(usage), 0, selected->GetSize());

        return selected;
    }

	// No suitable match found, create new one
	DataBufferGLES *res = UNITY_NEW(DataBufferGLES(*this, size, usage, clear), kMemGfxDevice);
	return res;
}

void BufferManagerGLES::ReleaseBuffer(DataBufferGLES* buffer)
{
    // A heuristic: Immediately release all static buffers, they're not supposed to be recycled anyway
#if 0
	// DISABLED FOR NOW. GUI.Label uses TextMeshGenerator which in turn uses
	// static meshes. This will mess up iOS. Instead, we'll just recycle these as well
	// and let the pruning mechanism get rid of them once the time comes.
	if (buffer->GetUsage() == DataBufferGLES::kStaticIBO ||
		buffer->GetUsage() == DataBufferGLES::kStaticUBO ||
		buffer->GetUsage() == DataBufferGLES::kStaticVBO)
	{
		UNITY_DELETE(buffer, kMemGfxDevice);
		return;
	}
#endif

	if (!BufferUpdateCausesStallGLES(buffer))
		InsertIntoLive(buffer);
	else
		m_pendingBuffers.push_back(buffer);
}

void BufferManagerGLES::AdvanceFrame()
{
	// Check earlier fences and create a new one
	if (GetGraphicsCaps().gles.hasFenceSync)
	{
		std::list<SyncFence>::iterator itr = m_LiveFences.begin();
		while(itr != m_LiveFences.end())
		{
			SyncFence &sf = *itr;
			GLenum ret = 0;
			GLES_CALL_RET(gGL, ret, glClientWaitSync, sf.m_Fence, 0, 0);
			if (ret == GL_ALREADY_SIGNALED)
			{
				// fence done, update last completed frame index.
				// New items are always added to the end, so we don't need to worry about overwriting a newer value
				m_LastCompletedFrameIndex = sf.m_FrameIndex;

				// Delete the sync and erase this one from the list
				GLES_CALL(gGL, glDeleteSync, sf.m_Fence);
				std::list<SyncFence>::iterator delItr = itr;
				itr++;
				m_LiveFences.erase(delItr);
			}
			else
			{
				// In case of multiple GL contexts, the oldest frame number might not finish first,
				// so only update the frame number when the oldest sync fence is done.
				break;
			}
		}
		// Sanity check (allowing 32-bit overflow)

		// Create a new sync
		GLsync fence = 0;
		GLES_CALL_RET(gGL, fence, glFenceSync, GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		SyncFence sf = { fence, m_frameIndex };
		m_LiveFences.push_back(sf);
	}
	else
		m_LastCompletedFrameIndex += 1;

	m_frameIndex += 1; // \note Overflow is ok.

	UpdateLiveSetFromPending();

	// \todo [2013-05-13 pyry] Do we want to do pruning somewhere else as well?
	if ((m_frameIndex % kBufferPruneFrequency) == 0)
		PruneFreeBuffers();
}

void BufferManagerGLES::UpdateLiveSetFromPending()
{
	int bufNdx = 0;
	while (bufNdx < (int)m_pendingBuffers.size())
	{
		if (!BufferUpdateCausesStallGLES(m_pendingBuffers[bufNdx]))
		{
			DataBufferGLES* newLiveBuffer = m_pendingBuffers[bufNdx];

			if (bufNdx+1 != (int)m_pendingBuffers.size())
				std::swap(m_pendingBuffers[bufNdx], m_pendingBuffers.back());
			m_pendingBuffers.pop_back();

			InsertIntoLive(newLiveBuffer);
			// \note bufNdx now contains a new buffer and it must be processed as well. Thus bufNdx is not incremented.
		}
		else
			bufNdx += 1;
	}
}

void BufferManagerGLES::InsertIntoLive(DataBufferGLES* buffer)
{
	m_liveBuffers[buffer->GetUsage()].insert(std::make_pair(buffer->GetSize(), buffer));
}

UInt32 BufferManagerGLES::GetTotalFreeSize()
{
	UInt32 totalBytes = 0;

	for (std::vector<DataBufferGLES*>::const_iterator bufIter = m_pendingBuffers.begin(); bufIter != m_pendingBuffers.end(); ++bufIter)
		totalBytes += (*bufIter)->GetSize();

	for (int ndx = 0; ndx < DataBufferGLES::kBufferUsageCount; ndx++)
	{
		for (std::multimap<int, DataBufferGLES*>::const_iterator bufIter = m_liveBuffers[ndx].begin(); bufIter != m_liveBuffers[ndx].end(); ++bufIter)
			totalBytes += (*bufIter).second->GetSize();
	}

	return totalBytes;
}


// Buffers are deleted at least when they've not been used for the last 59 frames (not 30 as it'd oscillate with the prune frequency)
// but may happen earlier as well for larger buffers (anything over 300kB gets deleted at 9 frames, scales down from there)

static const float kBufferDeleteThreshold	= 59.0f;
static const float kBufferMaxWeight			= (kBufferDeleteThreshold - 9.0f);
static const float kBufferDeleteWeight		= (kBufferMaxWeight / 300000.0f);

static float ComputeBufferDeleteWeight(const DataBufferGLES *buffer)
{
	const UInt32 renderAge = buffer->GetRenderAge();
	return float(renderAge) + std::min(kBufferMaxWeight, float(buffer->GetSize())*kBufferDeleteWeight);
}

void BufferManagerGLES::PruneFreeBuffers()
{
	DBG_LOG_BUF_GLES("BufferManagerGLES: %.2f MiB in free buffers", float(GetTotalFreeSize()) / float(1<<20));

	// \todo [2013-05-13 pyry] Do this properly - take into account allocated memory size.

	UInt32	numBytesFreed		= 0;
	int		numBuffersDeleted	= 0;

	// \note pending buffers are ignored. They will end up in live soon anyway.
	for (int usageClass = 0; usageClass < DataBufferGLES::kBufferUsageCount; usageClass++)
	{
		std::multimap<int, DataBufferGLES*>::iterator itr = m_liveBuffers[usageClass].begin();

		while (itr != m_liveBuffers[usageClass].end())
		{
			DataBufferGLES *buffer = (*itr).second;
			const float weight = ComputeBufferDeleteWeight(buffer);
			// Only delete buffer once it's done rendering, prevent stalls on Adrenos
			if (weight >= kBufferDeleteThreshold && !BufferUpdateCausesStallGLES(buffer))
			{
				std::multimap<int, DataBufferGLES*>::iterator del = itr;
				itr++;
				m_liveBuffers[usageClass].erase(del);
				// itr is still valid

				numBytesFreed += buffer->GetSize();
				numBuffersDeleted += 1;

				UNITY_DELETE(buffer, kMemGfxDevice);
			}
			else
				itr++;
		}
	}

	DBG_LOG_BUF_GLES("  => freed %d buffers, %u B / %.2f MiB", numBuffersDeleted, numBytesFreed, float(numBytesFreed) / float(1<<20));
}

void BufferManagerGLES::InvalidateAll()
{
	for (std::vector<DataBufferGLES*>::iterator iter = m_pendingBuffers.begin(); iter != m_pendingBuffers.end(); ++iter)
	{
		(*iter)->Disown();
		UNITY_DELETE(*iter, kMemGfxDevice);
	}
	m_pendingBuffers.clear();

	for (int usageClass = 0; usageClass < DataBufferGLES::kBufferUsageCount; usageClass++)
	{
		std::multimap<int, DataBufferGLES*>::iterator itr = m_liveBuffers[usageClass].begin();

		while (itr != m_liveBuffers[usageClass].end())
		{
			(*itr).second->Disown();
			UNITY_DELETE((*itr).second, kMemGfxDevice);
			itr++;
		}
		m_liveBuffers[usageClass].clear();
	}
}

BufferManagerGLES* g_bufferManager = 0;

BufferManagerGLES* GetBufferManagerGLES()
{
	if (!g_bufferManager)
		g_bufferManager = UNITY_NEW(BufferManagerGLES(), kMemGfxDevice);
	return g_bufferManager;
}

void ReleaseBufferManagerGLES()
{
	UNITY_DELETE(g_bufferManager, kMemGfxDevice);
	g_bufferManager = 0;
}

GfxDevice& GetRealGfxDevice();
