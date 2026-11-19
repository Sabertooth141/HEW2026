#include "GameScene.h"

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <Windows.h>
#include <cstdio>

#include "CameraController.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "TextRenderer.h"
#include "WindowSettings.h"
#include "RetryTransitionComponent.h"
#include "SoundManager.h"
#include "WaveData.h"
#include "AnimatorComponent.h"
#include "GameObject.h"
#include "RenderLayer.h"
#include "SpeedBoostComponent.h"
#include "PlayerController.h"

void GameScene::OnEnter()
{
	// ゲーム用BGMを読み込む。
	auto gameBgm = std::make_shared<WaveData>();
	std::string errorMessage;

	if (!gameBgm->Load(
		L"../../assets/sounds/Game_BGM.wav",
		errorMessage))
	{
		OutputDebugStringA("Game_BGM.wavの読み込みに失敗しました\n");
		OutputDebugStringA(errorMessage.c_str());
		OutputDebugStringA("\n");
	}
	else if (!context.soundManager.PlayBGM(gameBgm, 0.3f))
	{
		OutputDebugStringA("Game_BGM.wavの再生に失敗しました\n");
	}

	Scene::OnEnter();

	// 各ステージ配置位置。
	struct StageBackgroundData
	{
		const wchar_t* texturePath;
		const char* objectName;
		DirectX::XMFLOAT3 position;
	};

	const StageBackgroundData stageBackgroundDataList[] =
	{
		{ L"../../assets/mapdesign_s/1_s.png", "StageBackground1",{ 0.0f,     0.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/2_s.png", "StageBackground2",{ 1920.0f,     0.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/3_s.png", "StageBackground3",{ 3840.0f,     0.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/4_s.png", "StageBackground4",{ 3840.0f, -1080.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/5_s.png", "StageBackground5",{ 5760.0f, -1080.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/6_s.png", "StageBackground6",{ 5760.0f,     0.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/7_s.png", "StageBackground7",{ 7680.0f,     0.0f, 1.0f } },
		{ L"../../assets/mapdesign_s/8_s.png", "StageBackground8",{ 7680.0f, -1080.0f, 1.0f } }
	};								  


	// ステージ全体の背景画像を生成する。
	//背景なので、当たり判定やRigidbodyは付けない。
	GameObject* back = Add2DObject();

	if (back != nullptr)
	{
		back->SetName("StageBackground");

		auto& animator =
			back->AddComponent<AnimatorComponent>(back->GetRenderer());

		animator.SetRenderLayer(RenderLayer::BackGround);
		animator.SetSortOrder(0);
		animator.SetStatic(L"../../assets/haikei.png");

		// 5画面分の幅、2画面分の高さに合わせて配置する。
		// ステージ1の画面中心が (0, 0) の場合の座標。
		back->GetTransform()->SetPosition({ 3840.0f, -540.0f, 50.0f });
		back->GetTransform()->SetScale({ 9600.0f, 2160.0f, 1.0f });
	}
	// データに従ってステージオブジェクトを1枚ずつ生成する。
	for (const StageBackgroundData& stageData : stageBackgroundDataList)
	{
		GameObject* background = Add2DObject();

		if (background == nullptr)
		{
			continue;
		}

		background->SetName(stageData.objectName);
		background->GetTransform()->SetPosition(stageData.position);

		auto& animator =
			background->AddComponent<AnimatorComponent>(
				background->GetRenderer());

		animator.SetRenderLayer(RenderLayer::BackGround);
		animator.SetSortOrder(0);

		// ステージごとに対応する画像を読み込む。
		animator.SetStatic(stageData.texturePath);

		// 1枚の背景画像を画面サイズに合わせる。
		background->GetTransform()->SetScale(
			{ 1920.0f, 1080.0f, 1.0f });
	}
}
void GameScene::OnUpdate(float deltaTime)
{
}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);

	for (const auto& object : GetObjects())
	{
		auto* retryTransition =
			object->GetComponent<RetryTransitionComponent>();

		if (retryTransition != nullptr)
		{
			retryTransition->Draw();
		}
	}

	float ratio = 0.0f;

	for (const auto& object : GetObjects())
	{
		auto* player = object->GetComponent<PlayerController>();

		if (player != nullptr)
		{
			ratio = player->GetBoostRemainingRatio();
			break;
		}
	}

	constexpr int segmentCount = 20;

	//UTF-8の「■」
	constexpr char segment[] = "\xE2\x96\xA0";
	constexpr float scale = 1.5f;

	const float segmentWidth = text.Measure(segment, scale).x;
	const float spacing = 2.0f;

	const DirectX::XMFLOAT4 emptyColor = { 0.2f,0.2f,0.2f,1.0f };

	for (int i = 0;i < segmentCount;++i)
	{
		//残っている区間を点灯
		const bool filled = ratio > static_cast<float>(i) / segmentCount;

		text.DrawScreen(
			segment,
			{ 250.0f+i*(segmentWidth+spacing),160.0f},
			filled?TextColor::Green:emptyColor,
			scale,
			TextAlign::Left
		);
	}
}
void GameScene::OnExit()
{
	context.soundManager.StopBGM();
	Scene::OnExit();
}
