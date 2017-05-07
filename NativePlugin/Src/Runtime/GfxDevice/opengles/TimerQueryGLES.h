#pragma once

#if ENABLE_PROFILER

#include "ApiGLES.h"
#include "Runtime/GfxDevice/GfxTimerQuery.h"

class TimerQueryGLES;
class ApiGLES;

struct TimerQueriesGLES
{
	TimerQueryGLES *m_CurrentQueryList; //! A list of query objects that were measured during the current begin/end pair. The first entry is an internal one.
	TimerQueryGLES *m_CurrentQueryTail; //! Shortcut to the tail of the list above.

	TimerQueryGLES *m_FreeHeadsList; //! Linked list of available internal query objects to be used as mCurrentQueryList heads in case Begin/End is called multiple times in rapid succession.

	bool		Init(const ApiGLES & api);

	void		BeginTimerQueries();
	void		EndTimerQueries();

	TimerQueryGLES * GetNextObject(); // Get next free query head

	void		CheckForDisjoint();
};

extern TimerQueriesGLES g_TimerQueriesGLES;


class TimerQueryGLES : public GfxTimerQuery
{
	friend struct TimerQueriesGLES;
public:

	TimerQueryGLES();
	virtual ~TimerQueryGLES();

	virtual void				Measure();
	virtual ProfileTimeFormat	GetElapsed(UInt32 flags);

private:
	gl::QueryHandle		m_QueryHandle; // The GL timer query object.

	bool				m_IsDisjoint; // If true, timer disjointedness was detected, result invalid

	TimerQueryGLES *	m_Prev; // Pointer to previous timer query object, or NULL iff this is the timer query for BeginTimerQueries() call
	TimerQueryGLES *	m_Next; // Next item in list of queries performed this frame, or NULL if end of chain

	UInt64				m_QueryResult; // GL query result
	ProfileTimeFormat	m_Time;

	void				GetQueryResultImmediate();

};

#endif//ENABLE_PROFILER

