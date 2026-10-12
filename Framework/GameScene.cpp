#include "GameScene.h"

#include <format>

#include "CameraController.h"
#include "PhysicsTest.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "SpriteVertex.h"
#include "TextRenderer.h"
#include "EnemySpawner.h"
#include "WindowSettings.h"
#include "Mouse.h"

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

	// enemy
	auto* enemy = Instantiate(
		"enemy",
		{ 100.0f, -100.0f, 1.0f }
	);

	// ground
	// physics test
	//MeshData groundQuad = MakeSpriteQuad();
	//auto groundObj = Add2DObject();

	//groundObj->GetTransform()->SetPosition({ 0, -200, 1 });

	//groundObj->AddComponent<AnimatorComponent>(renderer);
	//groundObj->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Default);
	//groundObj->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	//groundObj->GetComponent<AnimatorComponent>()->SetStatic(L"../../assets/bgTest.jpg");
	//groundObj->GetTransform()->SetScale({ 1000, 100, 1 });

	//groundObj->AddComponent<PhysicsTest>();

	// physics
	//Rigidbody2DComponent* groundRb = &groundObj->AddComponent<Rigidbody2DComponent>(*groundObj->GetTransform(), 1.0f);
//	BoxCollider2D* groundCol = &groundObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f),
		//DirectX::XMFLOAT2(0, 0),
		//false, *groundObj->GetTransform());
	//groundRb->SetIsStatic(true);

	//playerObj->GetComponent<CameraController>()->SetGroundTop(groundObj->GetTransform()->GetPosition().y);

	//block->GetComponent<Rigidbody2DComponent>()->SetFreezeRotation(true);

	GameObject* spawnerObject = Add2DObject();
	auto& spawner = spawnerObject->AddComponent<EnemySpawner>();

	spawner.spawnInterval = 1.f;
	spawner.maxEnemies = 30;
	
	// レベルアップ選択肢UI
	for (int i = 0; i < 3; ++i)
	{
		levelUpChoiceObjects[i] = Add2DObject();

		auto& levelUpAnim =
			levelUpChoiceObjects[i]->AddComponent<AnimatorComponent>(context.renderer);

		levelUpAnim.SetRenderLayer(RenderLayer::UI);
		levelUpAnim.SetEnableScreenSpace(true);
		levelUpAnim.SetStatic(L"../../assets/LevelUp_MaxHP.png");
		levelUpAnim.SetEnabled(false);

    }
}

void GameScene::OnUpdate(float deltaTime)
{
	if (finished)
	{
		return;
	}

	auto* playerController =
		playerObj->GetComponent<PlayerController>();
	// プレイヤーのHPが0になったらゲームオーバー画面へ
	if (playerController != nullptr &&
		playerController->GetStatus().GetHP() <= 0.f)
	{
		finished = true;
		context.globalContext.towerHeight = GetTowerHeight();
		context.sceneManager->RequestChange("Result");
		return;
	}
	if (playerController != nullptr &&
		playerController->IsLevelUpSelecting())
	{
		// レベルアップ選択中は時間を止める
		HandleLevelUpInput();
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

const wchar_t* GetLevelUpImagePath(PlayerStatus::StatType type)
{
	switch (type)
	{
	case PlayerStatus::StatType::MAX_HP:
		return L"../../assets/LevelUp_MaxHP.png";

	case PlayerStatus::StatType::SWING_DAMAGE:
		return L"../../assets/LevelUp_SwingDamage.png";

	case PlayerStatus::StatType::SWING_SPEED:
		return L"../../assets/LevelUp_SwingSpeed.png";

	case PlayerStatus::StatType::MOVE_SPEED:
		return L"../../assets/LevelUp_MoveSpeed.png";

	case PlayerStatus::StatType::VIEW_RANGE:
		return L"../../assets/LevelUp_ViewRange.png";

	case PlayerStatus::StatType::HARPOON_INTERVAL:
		return L"../../assets/LevelUp_HarpoonInterval.png";

	case PlayerStatus::StatType::ABSORB_RANGE:
		return L"../../assets/LevelUp_AbsorbRange.png";
	}

	return L"";
}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);

	text.DrawScreen(
		std::format("タワーの高さ: {:.2f}", GetTowerHeight()),
		{ WIN_WIDTH * 0.5f, 24.f },
		TextColor::Yellow,
		1.f,
		TextAlign::Center);

	text.DrawScreen(
		std::format("残り時間: {:.2f}", std::max(playTimer, 0.f)),
		{ WIN_WIDTH * 0.5f, 40.f },
		TextColor::Yellow,
		1.f,
		TextAlign::Center);

	// プレイヤーのHP表示
	auto* hpPlayerController =
		playerObj->GetComponent<PlayerController>();

	if (hpPlayerController != nullptr)
	{
		const auto& status = hpPlayerController->GetStatus();

		text.DrawScreen(
			std::format("HP: {:.0f} / {:.0f}",
				status.GetHP(),
				status.GetMaxHP()),
			{ 20.f, 24.f },
			TextColor::Yellow,
			1.f,
			TextAlign::Left);
	}
	// レベルアップ選択中
	auto* playerController =
		playerObj->GetComponent<PlayerController>();

	if (playerController != nullptr &&
		playerController->IsLevelUpSelecting())
	{
		const auto& choices =
			playerController->GetLevelUpChoices();

		text.DrawScreen(
			"レベルアップ！",
			{ WIN_WIDTH * 0.5f, 150.f },
			TextColor::Yellow,
			2.f,
			TextAlign::Center);

		for (size_t i = 0; i < choices.size(); ++i)
		{
			const LevelUpChoiceRect rect = GetLevelUpChoiceRect(i);

			auto* anim =
				levelUpChoiceObjects[i]->GetComponent<AnimatorComponent>();

			anim->SetStatic(GetLevelUpImagePath(choices[i]));
			levelUpChoiceObjects[i]->GetTransform()->SetScale(0.5f);

			levelUpChoiceObjects[i]->GetTransform()->SetPosition({
				rect.centerX - WIN_WIDTH * 0.5f,
				WIN_HEIGHT * 0.5f - rect.centerY,
				1.f
				});

			anim->SetEnabled(true);
		}
	}
	else
	{
		for (int i = 0; i < 3; ++i)
		{
			auto* anim =
				levelUpChoiceObjects[i]->GetComponent<AnimatorComponent>();

			anim->SetEnabled(false);
		}
	}
}

float GameScene::GetTowerHeight() const
{
	CameraController* cam = playerObj->GetComponent<CameraController>();
	return cam->GetStackTopY() - cam->GetGroundTop();
}

void GameScene::HandleLevelUpInput()
{
	auto* playerController =
		playerObj->GetComponent<PlayerController>();

	if (playerController == nullptr)
	{
		return;
	}

	if (!context.mouse.LeftPressed())
	{
		return;
	}

	const float mouseX =
		static_cast<float>(context.mouse.GetPosX());

	const float mouseY =
		static_cast<float>(context.mouse.GetPosY());



	const auto& choices =
		playerController->GetLevelUpChoices();


	for (size_t i = 0; i < choices.size(); ++i)
	{
		const LevelUpChoiceRect rect = GetLevelUpChoiceRect(i);

		const float left = rect.centerX - rect.width * 0.5f;
		const float right = rect.centerX + rect.width * 0.5f;
		const float top = rect.centerY - rect.height * 0.5f;
		const float bottom = rect.centerY + rect.height * 0.5f;

		if (mouseX >= left &&
			mouseX <= right &&
			mouseY >= top &&
			mouseY <= bottom)
		{
			playerController->SelectLevelUpChoice(i);

			// レベルアップ選択肢の画像を消す
			for (int j = 0; j < 3; ++j)
			{
				auto* anim =
					levelUpChoiceObjects[j]->GetComponent<AnimatorComponent>();

				anim->SetEnabled(false);
			}

			playerController->IgnoreMouseUntilRelease();

			return;
		}
	}
}

float GameScene::GetViewRange() const
{
	auto* playerController =
		playerObj->GetComponent<PlayerController>();

	if (playerController == nullptr)
	{
		return 1.0f;
	}

	return playerController->GetStatus().GetViewRange();
}

LevelUpChoiceRect GameScene::GetLevelUpChoiceRect(size_t index) const
{
	LevelUpChoiceRect rect;

	rect.centerX = 180.f + index * 220.f;
	rect.centerY = 300.f;
	rect.width = 170.f;
	rect.height = 150.f;

	return rect;
}

bool GameScene::IsLevelUpSelecting() const
{
	auto* controller = playerObj->GetComponent<PlayerController>();
	return controller && controller->IsLevelUpSelecting();
}

