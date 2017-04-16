#pragma once

#include "Runtime/Threads/AtomicQueue.h"
#include "Runtime/Threads/ExtendedAtomicTypes.h"

class AtomicQueue;
class AtomicStack;

namespace Unity
{

	// This is a MultiReader single writer task list
	// task list that uses our JobQueue technology
	// NOTE: To use this T MUST look like an AtomicNode
	//       which means T cannot contain virtuals and
	//       the first data member must be volatile atomic_word _next;
	template< typename T >
	class TaskQueue
	{
	public:
		TaskQueue()
		{
			m_Queue = CreateAtomicQueue();
			m_NodePool = CreateAtomicStack();
		}

		~TaskQueue()
		{
			DestroyAtomicQueue(m_Queue);
			AtomicNode * node = m_NodePool->PopAll();
			while (node)
			{
				AtomicNode * next = node->Next();
				UNITY_FREE(kMemThread, (T*)node);
				node = next;
			}
			DestroyAtomicStack(m_NodePool);
		}

		// The task queue deals in operations
		// These are functors that do some work.
		void Enqueue(T * operation)
		{
			m_Queue->Enqueue((AtomicNode*)operation);
		}

		T * Dequeue()
		{
			return (T*)m_Queue->Dequeue();
		}

		T * GetTask()
		{
			AtomicNode * node = m_NodePool->Pop();
			if (NULL == node)
			{
				return (T*)UNITY_MALLOC_ALIGNED(kMemThread, sizeof(T), 16);
			}
			return (T*)node;

		}

		void FreeTask(T * node)
		{
			Assert(NULL != node);
			m_NodePool->Push((AtomicNode*)node);
		}

		AtomicQueue * m_Queue;
		AtomicStack * m_NodePool;
	};

	// This is a task type that will invoke
	// a method on the class of type T
	// as needed.
	template< class T >
	struct ClassAsyncTask
	{
	public:
		// Intrusive linked list AtomicNode requirement
		atomic_word _next;

		// Internal data of the structure.
		typedef void(T::*AsyncTaskOperation)();
		T * m_Self;
		AsyncTaskOperation  m_Op;

		// Actually invoke this operation on the class itself.
		void Run()
		{
			(*m_Self.*m_Op)();
		}

		// And build our grammar task
		ClassAsyncTask<T> * Init(T * self, AsyncTaskOperation op)
		{
			m_Self = self;
			m_Op = op;
			return this;
		}
	};

}