#pragma once

#include "Runtime/Threads/AtomicOps.h"
#include "Runtime/Threads/TaskQueue.h"
#include "Runtime/Threads/Thread.h"

namespace Unity
{

#define kTerminatedTask ((Task*)0xFFFFFFFFFFFFFFFF)
#define kInvalidTask ((Task*)0xBADC0FFEE0DDF00D)
#define kEmptyTask ((Task*)0x0)

template <typename T>
class TaskChainProcessor
{
public:
	typedef ClassAsyncTask<T> Task;

private:
	T& m_Commander;
	TaskQueue<Task> m_Tasks;
	volatile Task* m_CurrentAsyncTask;
	volatile bool m_IsTerminating;
	volatile bool m_IsCurrentTaskQueuedToMainThread;

	inline void FreeCurrentTaskIfNeeded()
	{
		if (m_CurrentAsyncTask != kTerminatedTask && m_CurrentAsyncTask != kInvalidTask && m_CurrentAsyncTask != kEmptyTask)
			m_Tasks.FreeTask((Task*)m_CurrentAsyncTask);
	}

	// shouldClear would typically be set to false when enqueueing new tasks,
	// and set to true when calling back to process next task from completion callback
	inline void ProcessNextTask(bool shouldClear)
	{
		// Try and flip to a terminated state so nothing will be able to run!
		if (m_IsTerminating)
		{
			if (shouldClear || AtomicCompareExchangePointer((void *volatile *)(&m_CurrentAsyncTask), kTerminatedTask, kEmptyTask))
			{
				FreeCurrentTaskIfNeeded();
				m_CurrentAsyncTask = kTerminatedTask;
			}

			return;
		}

		// We flip to something so nobody else will add to the queue while we are dequeing and trying to
		// run something. NOTE: In shouldClear state we are holding the lock effectively so we don't have to clear.
		if (shouldClear || AtomicCompareExchangePointer((void *volatile *)(&m_CurrentAsyncTask), kInvalidTask, kEmptyTask))
		{
			m_IsCurrentTaskQueuedToMainThread = false;
			Task* task = m_Tasks.Dequeue();
			if (NULL != task)
			{
				// Now we have our pointer we are going to run, so run it and cache that we are running it.
				FreeCurrentTaskIfNeeded();
				m_CurrentAsyncTask = task;
				task->Run();
			}
			else
			{
				// Do nothing, there are no tasks to run!
				FreeCurrentTaskIfNeeded();
				m_CurrentAsyncTask = kEmptyTask;
			}
		}
	}

public:
	inline TaskChainProcessor(T& commander) :
		m_Commander(commander),
		m_CurrentAsyncTask(kEmptyTask)
	{
	}

	inline ~TaskChainProcessor()
	{
		FreeCurrentTaskIfNeeded();
	}

	inline void Initialize()
	{
		m_IsTerminating = false;
		m_IsCurrentTaskQueuedToMainThread = false;
		FreeCurrentTaskIfNeeded();
		m_CurrentAsyncTask = kEmptyTask;
	}

	inline void Shutdown()
	{
		Assert(Thread::CurrentThreadIsMainThread());

		m_IsTerminating = true;

		// Try and flip to a terminated state so nothing will be able to run!
		AtomicCompareExchangePointer((void *volatile *)(&m_CurrentAsyncTask), kTerminatedTask, kEmptyTask);

		// Wait for current task to finish if there is one in flight!
		while (m_CurrentAsyncTask != kTerminatedTask)
		{
			// If m_IsCurrentTaskQueuedToMainThread, then it means a task was queued to the main thread
			// However, it will never get executed as we're on the mainthread ourselves
			if (m_IsCurrentTaskQueuedToMainThread)
			{
				FreeCurrentTaskIfNeeded();
				m_CurrentAsyncTask = kTerminatedTask;
				break;
			}

			Thread::Sleep(1);
		}
	}

	inline void ShutdownWithCleanup(typename Task::AsyncTaskOperation cleanupFunc, bool block)
	{
		Shutdown();
		m_IsTerminating = false;
		AddTask(cleanupFunc);
		ProcessNextTask(true);

		if (block)
		{
			Shutdown();
		}
		else
		{
			m_IsTerminating = true;
		}
	}

	inline void AddTask(typename Task::AsyncTaskOperation taskFunc)
	{
		Task* task = m_Tasks.GetTask()->Init(&m_Commander, taskFunc);
		m_Tasks.Enqueue(task);
		ProcessNextTask(false); // only start if nothing running
	}

	inline void ProcessNextTask()
	{
		ProcessNextTask(true);
	}

	inline bool HasPendingTasks() const
	{
		return m_CurrentAsyncTask != kEmptyTask;
	}

	inline void MarkCurrentTaskAsQueuedToMainThread()
	{
		AssertMsg(m_CurrentAsyncTask != kEmptyTask, "Marking empty task as queued to main thread");
		m_IsCurrentTaskQueuedToMainThread = true;
	}

	inline typename Task::AsyncTaskOperation GetCurrentOperation() const
	{
		if (m_CurrentAsyncTask == kEmptyTask)
			return nullptr;

		return m_CurrentAsyncTask->m_Op;
	}
};

}