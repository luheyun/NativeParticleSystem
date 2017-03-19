#pragma once

class ParticleSystemRenderer;
class ParticleSystemParticles;

class ParticleSystem
{
public:
	static void Init();
	static void BeginUpdateAll();
	static void EndUpdateAll();
	static void Prepare();
	static void Render();

	ParticleSystem();
	~ParticleSystem();

	bool IsActive() { return m_IsActive; }
	void PrepareForRender();

    int GetParticleCount() const;

private:
	ParticleSystemRenderer* m_Renderer;
    ParticleSystemParticles* m_Particles;
	bool m_IsActive = true;
};