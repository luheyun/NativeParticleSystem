#pragma once

#include "Threads/ExtendedAtomicTypes.h"
#include "Threads/AtomicNode.h"

#if defined(ATOMIC_HAS_QUEUE)

// A generic lockfree stack.
// Any thread can Push / Pop nodes to the stack.

// The stack is lockfree and highly optimized. It has different implementations for different architectures.

// On intel / arm it is built with double CAS:
// http://en.wikipedia.org/wiki/Double_compare-and-swap
// On PPC it is built on LL/SC:
// http://en.wikipedia.org/wiki/Load-link/store-conditional

class AtomicStack
{
#if defined(ATOMIC_HAS_DCAS)
	volatile atomic_word2   _top;
#else
	volatile atomic_word	_top;
#endif

public:
	AtomicStack();
	~AtomicStack();
	
	int IsEmpty() const;
	
	void Push(AtomicNode *node);
	void PushAll(AtomicNode *first, AtomicNode *last);

	AtomicNode *Pop();
	AtomicNode *PopAll();
};

AtomicStack* CreateAtomicStack ();
void DestroyAtomicStack (AtomicStack* s);


// A generic lockfree queue FIFO queue.
// Any thread can Enqueue / Dequeue in paralell.
// When pushing / popping a node there is no guarantee that the pointer to the node is the same (void* data[3])
// We do guarantee that all 3 data pointer are the same after deqeuing.


// The queue is lockfree and highly optimized. It has different implementations for different archectures.

// On intel / arm it is built with double CAS:
// http://en.wikipedia.org/wiki/Double_compare-and-swap
// On PPC it is built on LL/SC:
// http://en.wikipedia.org/wiki/Load-link/store-conditional

class AtomicQueue
{
#if defined(ATOMIC_HAS_DCAS)
	volatile atomic_word2   _tail;
#else
	volatile atomic_word	_tail;
#endif
	volatile atomic_word	_head;
	
public:
	AtomicQueue();
	~AtomicQueue();
	
	int IsEmpty() const;
	
	void Enqueue(AtomicNode *node);
	void EnqueueAll(AtomicNode *first, AtomicNode *last);
	AtomicNode *Dequeue();
};

AtomicQueue* CreateAtomicQueue ();
void DestroyAtomicQueue (AtomicQueue* s);

#endif


//
// Special concurrent list for JobQueue
// This code is not meant to be general purpose and should not be used outside of the job queue.

class AtomicList
{
#if defined(ATOMIC_HAS_DCAS)

	volatile atomic_word2	_top;

#else

	volatile atomic_word	_top;
	volatile atomic_word	_ver;
	
#endif

public:
	void Init();
	
	atomic_word Tag();
	AtomicNode *Peek();
	AtomicNode *Load(atomic_word &tag);
	
	AtomicNode *Clear(AtomicNode *old, atomic_word tag);
	
	bool Add(AtomicNode *first, AtomicNode *last, atomic_word tag);
	AtomicNode* Touch(atomic_word tag);
	void Reset(AtomicNode *node, atomic_word tag);
	
	static void Relax();
};
