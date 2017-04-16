#pragma once

// A simple single entry, lockless flag.
// There is no throttle to ensure the producer
// doesn't write multiple times before the consumer reads
// This object is most useful for simple status updates
// Where the producer writes the current state and the consumer
// wants to check if the current state has changed and do something
// with the change of status.
//
// Intended for single writer single reader!
//
template< typename T>
class ThreadedFlag
{
public:
	ThreadedFlag(T initialValue)
		: m_CurrentValue(initialValue)
		, m_NoticeValue(initialValue)
	{}

	bool CheckForStateChange( T & outCurrentValue )
	{
		// We assume this can work atomically for all platforms
		// with a 32 bit sized data type at least.
		Assert(sizeof(T) <= 32);
		
		// This handshake ensures the type stays
		// deterministic on the consuming thread.
		// since only the current thread should be
		// touching m_CurrentValue.
		T oldValue      = m_CurrentValue;
		m_CurrentValue  = m_NoticeValue;
		outCurrentValue = m_CurrentValue;
		return oldValue != m_CurrentValue;
	}

	void ProduceStateChange( T noticeValue )
	{
		m_NoticeValue = noticeValue;
	}

	T GetValue()
	{
		return m_CurrentValue;
	}

private:
	volatile T m_CurrentValue;
	volatile T m_NoticeValue;
};
