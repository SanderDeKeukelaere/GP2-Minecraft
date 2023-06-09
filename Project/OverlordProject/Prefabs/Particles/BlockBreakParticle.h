#pragma once
#include <Misc/World/WorldData.h>

class BlockBreakParticle final : public GameObject
{
public:
	void SetBlock(BlockType block);
	void SetActive(bool active);

protected:
	virtual void Initialize(const SceneContext& sceneContext) override;

private:
	std::vector<ParticleEmitterComponent*> m_pEmitters{};
	BlockType m_Block{};

	float m_MaxTime{ 1.0f };
	float m_CurTime{};
};

