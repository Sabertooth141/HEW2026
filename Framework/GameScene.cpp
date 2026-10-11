#include "GameScene.h"

#include <format>

#include "CameraController.h"
#include "PhysicsTest.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "SpriteVertex.h"
#include "TextRenderer.h"
#include "EnemySpawner.h"

void GameScene::OnEnter()
{
	Scene::OnEnter();
	Renderer& renderer = context.renderer;

	// player
	MeshData quad = MakeSpriteQuad();
	playerObj = Add2DObject();
	playerObj->SetTag(ObjectTag::PLAYER);

	playerObj->GetTransform()->SetPosition({ -128, -100, 1 });

	// physics
	Rigidbody2DComponent* playerRb = &playerObj->AddComponent<Rigidbody2DComponent>(*playerObj->GetTransform(), 1.0f);
	BoxCollider2D* col = &playerObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(1, 1), DirectX::XMFLOAT2(0, 0), true,
		*playerObj->GetTransform());
	playerRb->SetFreezeRotation(true);
	playerRb->SetGravity(0.f);

	// animation
	playerObj->AddComponent<AnimatorComponent>(renderer);
	playerObj->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Player);
	playerObj->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	playerObj->GetComponent<AnimatorComponent>()->AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png",
		L"../../assets/PlayerCharacter.json");
	playerObj->GetComponent<AnimatorComponent>()->AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
		L"../../assets/PlayerCharacterMove.json");
	playerObj->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharIdle");

	// script
	playerObj->AddComponent<PlayerController>();
	playerObj->GetComponent<PlayerController>()->SetInput(context.keyboard, context.mouse);
	playerObj->AddComponent<CameraController>();

	// ground
	// physics test
	MeshData groundQuad = MakeSpriteQuad();
	auto groundObj = Add2DObject();

	groundObj->GetTransform()->SetPosition({ 0, -200, 1 });

	groundObj->AddComponent<AnimatorComponent>(renderer);
	groundObj->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Default);
	groundObj->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	groundObj->GetComponent<AnimatorComponent>()->SetStatic(L"../../assets/bgTest.jpg");
	groundObj->GetTransform()->SetScale({ 1000, 100, 1 });

	groundObj->AddComponent<PhysicsTest>();

	// physics
	Rigidbody2DComponent* groundRb = &groundObj->AddComponent<Rigidbody2DComponent>(*groundObj->GetTransform(), 1.0f);
	BoxCollider2D* groundCol = &groundObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f),
		DirectX::XMFLOAT2(0, 0),
		false, *groundObj->GetTransform());
	groundRb->SetIsStatic(true);

	playerObj->GetComponent<CameraController>()->SetGroundTop(groundObj->GetTransform()->GetPosition().y);

	//block->GetComponent<Rigidbody2DComponent>()->SetFreezeRotation(true);

	GameObject* spawnerObject = Add2DObject();
	auto& spawner = spawnerObject->AddComponent<EnemySpawner>();

	spawner.spawnInterval = 1.f;
	spawner.maxEnemies = 30;
}

void GameScene::OnUpdate(float deltaTime)
{
	if (finished)
	{
		return;
	}

	playTimer -= deltaTime;
	if (playTimer <= 0.f)
	{
		finished = true;
		context.globalContext.towerHeight = GetTowerHeight();
		context.sceneManager->RequestChange("Result");
	}
}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);

	text.DrawScreen(std::format("タワーの高さ: {:.2f}", GetTowerHeight()),
		{ WIN_WIDTH * 0.5f, 24.f }, TextColor::Yellow, 1.f, TextAlign::Center);

	text.DrawScreen(std::format("残り時間: {:.2f}", std::max(playTimer, 0.f)),
		{ WIN_WIDTH * 0.5f, 40.f }, TextColor::Yellow, 1.f, TextAlign::Center);
}

float GameScene::GetTowerHeight() const
{
	CameraController* cam = playerObj->GetComponent<CameraController>();
	return cam->GetStackTopY() - cam->GetGroundTop();
}
