#include "stdafx.h"
#include "WorldScene.h"

#include "Components/WorldComponent.h"
#include "Components/PlayerMovement.h"

void WorldScene::Initialize()
{
	auto& physX{ PxGetPhysics() };
	auto pPhysMat{ physX.createMaterial(0.0f, 0.0f, 0.0f) };

	CreateWorld();

	CreatePlayer(pPhysMat);
}

void WorldScene::CreateWorld()
{
	GameObject* pWorld{ AddChild(new GameObject{}) };
	pWorld->AddComponent(new WorldComponent{ m_SceneContext });
}

void WorldScene::CreatePlayer(physx::PxMaterial* pPhysMat)
{
	// Create player
	GameObject* pPlayer{ AddChild(new GameObject{}) };

	// RIGIDBODY
	RigidBodyComponent* pPlayerRb{ pPlayer->AddComponent(new RigidBodyComponent{}) };
	// Add collider
	const PxBoxGeometry playerGeometry{ 0.3f, 1.0f, 0.25f };
	pPlayerRb->AddCollider(playerGeometry, *pPhysMat);
	// Lock all rotations
	pPlayerRb->SetConstraint(RigidBodyConstraint::RotX | RigidBodyConstraint::RotY | RigidBodyConstraint::RotZ, false);
	// Double gravity
	PxScene* physScene{ pPlayerRb->GetPxRigidActor()->getScene() };
	physScene->setGravity(physScene->getGravity() * 2);

	// MOVEMENT
	pPlayer->AddComponent(new PlayerMovement{ pPlayerRb });

	// POSITION
	pPlayer->GetTransform()->Translate(0.0f, 100.0f, 0.0f);



	// Create camera
	GameObject* pCameraGO{ pPlayer->AddChild(new GameObject{}) };

	// CAMERA
	CameraComponent* pCamera{ pCameraGO->AddComponent(new CameraComponent{}) };
	pCamera->SetFieldOfView(90.0f);
	SetActiveCamera(pCamera); //Also sets pCamera in SceneContext

	// POSITION
	pCameraGO->GetTransform()->Translate(0.0f, 0.5f, 0.0f);
}

void WorldScene::Update()
{
}

void WorldScene::Draw()
{
}

void WorldScene::OnGUI()
{
}
