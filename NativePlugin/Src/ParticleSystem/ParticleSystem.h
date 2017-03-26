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
    static void Update0();

	ParticleSystem();
	~ParticleSystem();

	bool IsActive() { return m_IsActive; }
	void PrepareForRender();

    void RemoveFromManager();

    int GetParticleCount() const;

private:
    int m_EmittersIndex;
	ParticleSystemRenderer* m_Renderer;
    ParticleSystemParticles* m_Particles;
	bool m_IsActive = true;
};