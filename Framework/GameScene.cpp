#include "GameScene.h"

#include <algorithm>
#include <format>

#include "CameraController.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "TextRenderer.h"
#include "WindowSettings.h"

void GameScene::OnEnter()
{
	Scene::OnEnter();

	// player
	playerObj = Add2DObject();
	playerObj->SetTag(ObjectTag::PLAYER);
	playerObj->GetTransform()->SetPosition({ -128.f, -100.f, 1.f });

	// physics
	auto& playerRb = playerObj->AddComponent<Rigidbody2DComponent>(*playerObj->GetTransform(), 1.f);
	playerRb.SetFreezeRotation(true);
	playerRb.SetGravity(0.f);
	playerObj->AddComponent<BoxCollider2D>(
		DirectX::XMFLOAT2(1.f, 1.f), DirectX::XMFLOAT2(0.f, 0.f), true, *playerObj->GetTransform());

	// animation
	auto& anim = playerObj->AddComponent<AnimatorComponent>(context.renderer);
	anim.SetRenderLayer(RenderLayer::Player);
	anim.SetSortOrder(0);
	anim.AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png", L"../../assets/PlayerCharacter.json");
	anim.AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png", L"../../assets/PlayerCharacterMove.json");
	anim.SetCurrAnimation("CharIdle");

	// script
	auto& controller = playerObj->AddComponent<PlayerController>();
	controller.SetInput(context.keyboard, context.mouse);
	playerObj->AddComponent<CameraController>();

	// enemies (prefab)
	Instantiate("enemy", { 200.f, -100.f, 1.f });
	Instantiate("enemy", { -300.f, 150.f, 1.f });
}

void GameScene::OnUpdate(float deltaTime)
{
	if (finished)
	{
		return;
	}

	playTimer += deltaTime;

	// プレイヤーのHPが0になったらResultへ
	const auto* controller = playerObj->GetComponent<PlayerController>();
	if (controller != nullptr && controller->IsDead())
	{
		finished = true;
		context.globalContext.playTime = playTimer;
		context.sceneManager->RequestChange("Result");
	}
}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);

	if (const auto* controller = playerObj->GetComponent<PlayerController>())
	{
		text.DrawScreen(std::format("HP: {:.0f} / {:.0f}", controller->GetHP(), controller->GetMaxHP()),
			{ 20.f, 24.f }, TextColor::Yellow, 1.f, TextAlign::Left);
	}

	text.DrawScreen(std::format("プレイ時間: {:.2f}", std::max(playTimer, 0.f)),
		{ WIN_WIDTH * 0.5f, 40.f }, TextColor::Yellow, 1.f, TextAlign::Center);
}
