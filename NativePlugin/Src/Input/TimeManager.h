#pragma once

class TimeManager
{
public:
	inline double GetCurTime()  const { return m_CurFrameTime; }
	inline float GetDeltaTime() const { return m_DeltaTime; }
	void SetDeltaTime(float deltaTime) { m_DeltaTime = deltaTime; }
	void SetFrameTime(float frameTime) { m_CurFrameTime = frameTime; }

private:
	float m_DeltaTime;
	double m_CurFrameTime;
};

TimeManager* g_TimeManager = new TimeManager();
inline float GetDeltaTime() { return g_TimeManager->GetDeltaTime(); }
inline double GetCurTime()	{ return g_TimeManager->GetCurTime(); }
inline void SetDeltaTime(float deltaTime) { g_TimeManager->SetDeltaTime(deltaTime); }
inline void SetFrameTime(float frameTime) { g_TimeManager->SetFrameTime(frameTime); }