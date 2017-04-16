#pragma once
#include "Runtime/Threads/AtomicNode.h"
#include "Runtime/Threads/Mutex.h"

class MutexLockedStack
{
	AtomicNode* m_Top;
	mutable Mutex m_Lock;

public:
	MutexLockedStack()
		: m_Top(NULL)
	{
	}
	~MutexLockedStack()
	{
	}
	
	int IsEmpty() const
	{
		Mutex::AutoLock lock(m_Lock);

		return m_Top == NULL ? 1 : 0;
	}
	
	void Push(AtomicNode *node)
	{
		Mutex::AutoLock lock(m_Lock);

		node->_next = (atomic_word)m_Top;
		m_Top = node;
	}
	
	void PushAll(AtomicNode *first, AtomicNode *last)
	{
		Mutex::AutoLock lock(m_Lock);

		AtomicNode* _current = first;
		while(_current != last && _current != NULL)
		{
			Push(_current);
			_current = (AtomicNode*)_current->_next;
		}
		Push(last);
	}

	AtomicNode *Pop()
	{
		Mutex::AutoLock lock(m_Lock);

		AtomicNode* r = m_Top;
		if (m_Top != NULL)
			m_Top = (AtomicNode*)m_Top->_next;
		return r;
	}

	AtomicNode *PopAll()
	{
		Mutex::AutoLock lock(m_Lock);

		AtomicNode* _last = NULL;
		while(IsEmpty()==0)
		{
			_last = Pop();
		}
		return _last;
	}
};

MutexLockedStack* CreateMutexLockedStack ();
void DestroyMutexLockedStack (MutexLockedStack* s);
