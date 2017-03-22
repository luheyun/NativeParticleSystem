#include "PluginPrefix.h"
#include "ParticleSystem.h"
#include "ParticleSystem/ParticleSystemRenderer.h"

struct ParticleSystemManager
{
	ParticleSystemManager()
	{
		emitters.reserve(2);
	}

	std::vector<ParticleSystem*> emitters;
};

ParticleSystemManager* gParticleSystemManager = nullptr;

void ParticleSystem::Init()
{
	gParticleSystemManager = new ParticleSystemManager();
}

void ParticleSystem::BeginUpdateAll()
{

}

void ParticleSystem::EndUpdateAll()
{

}

ParticleSystem::ParticleSystem()
	: m_Renderer(nullptr)
{
	gParticleSystemManager->emitters.push_back(this);
	m_Renderer = new ParticleSystemRenderer();
}

ParticleSystem::~ParticleSystem()
{
	auto it = std::find(gParticleSystemManager->emitters.begin(), gParticleSystemManager->emitters.end(), this);
	gParticleSystemManager->emitters.erase(it);

	if (m_Renderer != nullptr)
		delete m_Renderer;

	m_Renderer = nullptr;
}

void ParticleSystem::Prepare()
{
	auto it = gParticleSystemManager->emitters.begin();

	for (; it != gParticleSystemManager->emitters.end(); ++it)
		(*it)->PrepareForRender();
}

void ParticleSystem::Render()
{
	auto it = gParticleSystemManager->emitters.begin();

	for (; it != gParticleSystemManager->emitters.end(); ++it)
		(*it)->m_Renderer->RenderMultiple();
}

void ParticleSystem::PrepareForRender()
{
	m_Renderer->PrepareForRender(*this);
}

int ParticleSystem::GetParticleCount() const
{
    return 1;
}