#pragma once
#if ENABLE_MULTITHREADED_CODE

#include "Runtime/GfxDevice/GfxDevice.h"
#include "Runtime/Utilities/dynamic_array.h"
#include "Runtime/Threads/MultiWriterSingleReaderAtomicCircularBuffer.h"
#include "Runtime/Threads/Mutex.h"
#include "Runtime/Threads/Semaphore.h"
#include "Runtime/Threads/ConcurrentContainers.h"
#include "Runtime/Graphics/Mesh/MeshSkinning.h"
#include <deque>


#define GFXDEVICE_USE_CACHED_STATE 0
#define DEBUG_GFXDEVICE_LOCKSTEP 0

#if DEBUG_GFXDEVICE_LOCKSTEP
	#define GFXDEVICE_LOCKSTEP_CLIENT() { DoLockstep(); }
	#define GFXDEVICE_LOCKSTEP_WORKER() { DoLockstep(pos, cmd); }
#else
	#define GFXDEVICE_LOCKSTEP_CLIENT()
	#define GFXDEVICE_LOCKSTEP_WORKER()
#endif

// DXGI Present sends messages when reentering full-screen exclusive mode.
// Unity's message pump must be active in that case, or else client/worker deadlocks.
// More info: https://msdn.microsoft.com/en-us/library/windows/desktop/ee417025%28v=vs.85%29.aspx#multithreading_and_dxgi
#define GFXDEVICE_WAITFOREVENT_MESSAGEPUMP (UNITY_STANDALONE && UNITY_WIN) // Fixes case 523691

struct ClientDeviceTimerQuery;
struct ClientDeviceBlendState;
struct ClientDeviceDepthState;
struct ClientDeviceStencilState;
struct ClientDeviceRasterState;
struct VertexChannelsInfo;
class ThreadedStreamBuffer;
class ThreadedDisplayList;
class ThreadedVertexDeclaration;
class Thread;

// Multiple producer single consumer
class CreateGpuProgramQueue
{
public:
	struct Command
	{
		Command(ShaderGpuProgramType _type, const dynamic_array<UInt8>& _source, CreateGpuProgramOutput* _output, GpuProgram** _result)
		{
			type=_type;
			source.assign(_source.begin(), _source.end());
			output=_output;
			result=_result;
			sema.Reset();
		}

		ShaderGpuProgramType type;
		dynamic_array<UInt8> source;
		CreateGpuProgramOutput* output;
		GpuProgram** result;
		Semaphore sema;
	
	
		static Command* Create(ShaderGpuProgramType type, const dynamic_array<UInt8>& source, CreateGpuProgramOutput* output, GpuProgram** result) { return UNITY_NEW (Command, kMemTempJobAlloc)(type, source, output, result); }
		static void Destroy(Command* cmd) { UNITY_DELETE (cmd, kMemTempJobAlloc); }
	};

	CreateGpuProgramQueue() {}

	void Init(int maxSize) ;
	void Cleanup() ;
	Command* Enqueue(ShaderGpuProgramType shaderProgramType, const dynamic_array<UInt8>& source, CreateGpuProgramOutput* output, GpuProgram** result);
	void DequeueAll(GfxThreadableDevice* device);
	
protected:
	ConcurrentQueue* m_Queue; 
};

class GfxDeviceWorker : public NonCopyable
{
public:
	GfxDeviceWorker(int maxCallDepth, ThreadedStreamBuffer* commandQueue);
	virtual ~GfxDeviceWorker();

	GfxThreadableDevice* Startup(GfxDeviceRenderer renderer, bool threaded, bool forceRef);
#if ENABLE_UNIT_TESTS && GFX_SUPPORTS_NULL
	void StartupTests(GfxThreadableDevice* renderer, bool threaded, bool forceRef);
#endif

	void	WaitForSignal();
	void	LockstepWait();

	void	GetLastFrameStats(GfxDeviceStats& stats);

	void	CallImmediate(ThreadedDisplayList* dlist);

	enum EventType
	{
		kEventTypePresent,
		kEventTypeTimerQueries,
		kEventTypeCPUFence,
		kEventTypeCount
	};

	void	WaitForEvent(EventType type);
#if GFXDEVICE_WAITFOREVENT_MESSAGEPUMP
	void	WaitForEventWithMessagePump(EventType type);
#endif

	void	WaitOnCPUFence(UInt32 fence);

	bool	DidPresentFrame(UInt32 frameID) const;

	GfxThreadableDevice * GetThreadableDevice() { return m_Device; }

	CreateGpuProgramQueue& GetCreateGPUProgramQueue() { return m_CreateGpuProgramQueue; }

protected:
	// PLATFORM SPECIFIC OVERRIDABLES
	// This gets called for any commands that are not common to all platforms.
	// platform specific extensions go in here.
	virtual void            RunCommandExt( int lastCmd, int cmd, ThreadedStreamBuffer &stream);
	virtual int             GetDeviceWorkerProcessor();
	virtual ThreadPriority  GetDeviceWorkerPriority();
	virtual void            Run();
	
	void	SignalEvent(EventType type);

	static void* RunGfxDeviceWorker(void *data);

	void RunCommand(ThreadedStreamBuffer& stream);

	void Signal();
	void DoLockstep(int pos, int cmd);

	const void* ReadBufferData(ThreadedStreamBuffer& stream, int size, bool asPointer = false);
	void ReadBufferDataComplete();

#if ENABLE_PROFILER
	void PollTimerQueries();
	bool PollNextTimerQuery(bool wait);
#endif

#if GFXDEVICE_USE_CACHED_STATE
	struct CachedState
	{
		CachedState();
		const ClientDeviceBlendState* blendState;
		const ClientDeviceDepthState* depthState;
		const ClientDeviceStencilState* stencilState;
		int stencilRef;
		const ClientDeviceRasterState* rasterState;
		int backface;
	};
#endif

	dynamic_array<UInt8, 16> m_TempBuffer;
	Semaphore m_EventSemaphores[kEventTypeCount];
	Semaphore m_LockstepSemaphore;
	Semaphore m_WaitSemaphore;
	Mutex m_StatsMutex;
	GfxDeviceStats m_FrameStats;

#if GFXDEVICE_USE_CACHED_STATE
	CachedState m_Cached;
#endif
#if ENABLE_PROFILER
	// Timer queries for GPU profiling
	typedef std::deque<ClientDeviceTimerQuery*> TimerQueryList;
	TimerQueryList m_PolledTimerQueries;
#endif

	GfxThreadableDevice * m_Device;
	Thread              * m_WorkerThread;
	ThreadedStreamBuffer* m_CommandQueue;
	ThreadedStreamBuffer* m_MainCommandQueue;
	ThreadedStreamBuffer** m_PlaybackCommandQueues;
	ThreadedStreamBuffer* m_PlaybackCommandQueueReturn;
	ThreadedDisplayList** m_PlaybackDisplayLists;
	int                   m_CallDepth;
	int                   m_MaxCallDepth;
	volatile UInt32       m_CurrentCPUFence;
	volatile UInt32       m_PresentFrameID;
	bool                  m_IsThreadOwner;
	bool                  m_Quit;

	CreateGpuProgramQueue m_CreateGpuProgramQueue;
};

#endif
