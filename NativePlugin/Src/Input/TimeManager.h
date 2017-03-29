#pragma once

class TimeManager
{
public:
	inline double GetCurTime()  const { return m_CurFrameTime; }
	inline float GetDeltaTime() const { return m_DeltaTime; }
	void SetDeltaTime(float deltaTime) { m_DeltaTime = deltaTime; }

private:
	float m_DeltaTime;
	double m_CurFrameTime;
};

TimeManager* g_TimeManager = new TimeManager();
inline float GetDeltaTime() { return g_TimeManager->GetDeltaTime(); }
inline double GetCurTime()	{ return g_TimeManager->GetCurTime(); }