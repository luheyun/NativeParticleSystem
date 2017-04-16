#ifndef THREAD_SHARED_OBJECT_H
#define THREAD_SHARED_OBJECT_H

#include "Runtime/Threads/AtomicOps.h"

// TODO: Make it a template class and remove the virtual destructor
// template <typename T, MemLabelRef DefaultMemLabel>
class ThreadSharedObject
{
public:
	void AddRef() const	{ AtomicIncrement(&m_RefCount); }
	void Release() const { if (AtomicDecrement(&m_RefCount) == 0) delete this; }
	void Release(MemLabelRef label) const
	{
		if (AtomicDecrement(&m_RefCount) == 0)
		{
			this->~ThreadSharedObject();
			UNITY_FREE(label,const_cast<ThreadSharedObject*>(this));
		}
	}
	int  GetRefCount() const { return m_RefCount; }

protected:
	ThreadSharedObject() : m_RefCount(1) {}
	ThreadSharedObject(const ThreadSharedObject&) : m_RefCount(1) {}
	virtual ~ThreadSharedObject() {}

private:
	ThreadSharedObject& operator=(const ThreadSharedObject&); // No assignment

	volatile mutable int m_RefCount;
};

#endif
