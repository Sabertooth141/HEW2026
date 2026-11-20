#include "GameScene.h"

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <Windows.h>

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

	// 各ステージ背景の画像と配置位置。
	struct StageBackgroundData
	{
		const wchar_t* texturePath;
		const char* objectName;
		DirectX::XMFLOAT3 position;
	};

	const StageBackgroundData stageBackgroundDataList[] =
	{
		{ L"../../assets/mapdesign/1.png", "StageBackground1",{ 0.0f,     0.0f, 10.0f } },
		{ L"../../assets/mapdesign/2.png", "StageBackground2",{ 1920.0f,     0.0f, 10.0f } },
		{ L"../../assets/mapdesign/3.png", "StageBackground3",{ 3840.0f,     0.0f, 10.0f } },
		{ L"../../assets/mapdesign/4.png", "StageBackground4",{ 3840.0f, -1080.0f, 10.0f } },
		{ L"../../assets/mapdesign/5.png", "StageBackground5",{ 5760.0f, -1080.0f, 10.0f } },
		{ L"../../assets/mapdesign/6.png", "StageBackground6",{ 5760.0f,     0.0f, 10.0f } },
		{ L"../../assets/mapdesign/7.png", "StageBackground7",{ 7680.0f,     0.0f, 10.0f } },
		{ L"../../assets/mapdesign/8.png", "StageBackground8",{ 7680.0f, -1080.0f, 10.0f } }
	};

	// データに従って背景オブジェクトを1枚ずつ生成する。
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
		auto* retryTransition = object->GetComponent<RetryTransitionComponent>();

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
	constexpr float scale = 2.0f;

	const float segmentWidth = text.Measure(segment, scale).x;
	const float spacing = 2.0f;

	const DirectX::XMFLOAT4 emptyColor = { 0.2f,0.2f,0.2f,1.0f };

	for (int i = 0;i < segmentCount;++i)
	{
		//残っている区間を点灯
		const bool filled = ratio > static_cast<float>(i) / segmentCount;

		text.DrawScreen(
			segment,
			{ 350.0f+i*(segmentWidth+spacing),190.0f},
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
