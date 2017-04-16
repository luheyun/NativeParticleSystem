#pragma once

#include "Runtime/Threads/AtomicNode.h"
#include "Runtime/Threads/Mutex.h"

class MutexLockedQueue
{
public:

	MutexLockedQueue()
    {
        AtomicNode* node = UNITY_NEW (AtomicNode, kMemThread);
        node->_next = 0;
        m_Tail = m_Head = node;
    }
	~MutexLockedQueue()
    {
        UNITY_DELETE(m_Head, kMemThread);
    }
	
	int IsEmpty()
    {
        Mutex::AutoLock autoLock(m_Lock);
        
        return (m_Tail->_next == 0);
    }
	
	void Enqueue(AtomicNode *node)
    {
        Mutex::AutoLock autoLock(m_Lock);
	    
        node->_next = 0;
        AtomicNode* prev = m_Head;
        m_Head = node;
	    prev->_next = (atomic_word) node;
    }

	void EnqueueAll(AtomicNode *first, AtomicNode *last)
    {
        Mutex::AutoLock autoLock(m_Lock);

        last->_next = 0;
        AtomicNode* prev = m_Head;
        m_Head = last;
	    prev->_next = (atomic_word) first;
    }

	AtomicNode *Dequeue()
    {
        Mutex::AutoLock autoLock(m_Lock);

	    AtomicNode* res,* next;
	    
		res = m_Tail;
		next = (AtomicNode*) m_Tail->_next;
		if (next == 0) return NULL;
        // Satisfy Unit Tests. This test too MUST run fine with AtomicQueueStressTest.cpp
		res->data[0] = next->data[0];
        res->data[1] = next->data[1];
        res->data[2] = next->data[2];
        m_Tail = next;

	    return res;
    }

private:

	AtomicNode* m_Head;
    AtomicNode* m_Tail;
    Mutex m_Lock;

};

MutexLockedQueue* CreateMutexLockedQueue();
void DestroyMutexLockedQueue (MutexLockedQueue* queue);