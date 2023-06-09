#pragma once

class WorldComponent;
class BlockInteractionComponent;
class BlockBreakParticle;
class PlayerMovement;

class Player final : public GameObject
{
public:
	Player(WorldComponent* pWorld, GameObject* pSelection, BlockBreakParticle* pBlockBreakParticle);
	~Player() override = default;

	Player(const Player& other) = delete;
	Player(Player&& other) noexcept = delete;
	Player& operator=(const Player& other) = delete;
	Player& operator=(Player&& other) noexcept = delete;

	bool IsUnderWater() const;

protected:
	void Initialize(const SceneContext& sceneContext) override;
	void Update(const SceneContext& sceneContext) override;

private:
	BlockInteractionComponent* m_pInteraction{};
	WorldComponent* m_pWorld{};
	GameObject* m_pSelection{};
	BlockBreakParticle* m_pBlockBreakParticle{};
	ModelAnimator* m_pArmAnimation{};
	PlayerMovement* m_pMovement{};

	FMOD::Sound* m_pFallSound{};
	FMOD::Channel* m_pFallSoundChannel{};
	FMOD::Sound* m_pDamageSound{};
	FMOD::Channel* m_pDamageChannel{};

	GameObject* m_pFeet{};

	bool m_IsCamUnderWater{};
};
