#include "PluginPrefix.h"
#include "ParticleSystem.h"
#include "ParticleSystem/ParticleSystemRenderer.h"

struct ParticleSystemManager
{
	ParticleSystemManager()
	{
		activeEmitters.reserve(2);
	}

	std::vector<ParticleSystem*> activeEmitters;
};

ParticleSystemManager* gParticleSystemManager = nullptr;

void ParticleSystem::Init()
{
	gParticleSystemManager = new ParticleSystemManager();
}

void ParticleSystem::BeginUpdateAll()
{
    for (size_t i = 0; i < gParticleSystemManager->activeEmitters.size(); ++i)
    {
        ParticleSystem& system = *gParticleSystemManager->activeEmitters[i];

        if (!system.IsActive())
        {
            system.RemoveFromManager();
            continue;
        }

        Update0();
    }
}

void ParticleSystem::EndUpdateAll()
{

}

ParticleSystem::ParticleSystem()
	: m_Renderer(nullptr)
    , m_EmittersIndex(-1)
{
	gParticleSystemManager->activeEmitters.push_back(this);
	m_Renderer = new ParticleSystemRenderer();
}

ParticleSystem::~ParticleSystem()
{
	auto it = std::find(gParticleSystemManager->activeEmitters.begin(), gParticleSystemManager->activeEmitters.end(), this);
	gParticleSystemManager->activeEmitters.erase(it);

	if (m_Renderer != nullptr)
		delete m_Renderer;

	m_Renderer = nullptr;
}

void ParticleSystem::Prepare()
{
	auto it = gParticleSystemManager->activeEmitters.begin();

	for (; it != gParticleSystemManager->activeEmitters.end(); ++it)
		(*it)->PrepareForRender();
}

void ParticleSystem::Render()
{
	auto it = gParticleSystemManager->activeEmitters.begin();

	for (; it != gParticleSystemManager->activeEmitters.end(); ++it)
		(*it)->m_Renderer->RenderMultiple();
}

void ParticleSystem::Update0()
{

}

void ParticleSystem::PrepareForRender()
{
	m_Renderer->PrepareForRender(*this);
}

void ParticleSystem::RemoveFromManager()
{
    if (m_EmittersIndex < 0)
        return;

    const int index = m_EmittersIndex;
    gParticleSystemManager->activeEmitters[index]->m_EmittersIndex = -1;
    gParticleSystemManager->activeEmitters[index] = gParticleSystemManager->activeEmitters.back();

    if (gParticleSystemManager->activeEmitters[index] != this)
        gParticleSystemManager->activeEmitters[index]->m_EmittersIndex = index;

    gParticleSystemManager->activeEmitters.pop_back();
}

int ParticleSystem::GetParticleCount() const
{
    return 1;
}