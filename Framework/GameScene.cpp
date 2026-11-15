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
	// ゲーム用BGMを読み込む
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
	// プレハブを指定位置・サイズで生成する共通処理
	const auto createObject =
		[this](
			const std::string& prefabName,
			const std::string& objectName,
			const DirectX::XMFLOAT3& position,
			const DirectX::XMFLOAT3& scale)
		{
			GameObject* object = Instantiate(prefabName, position);
			if (object == nullptr)
			{
				return;
			}

			object->SetName(objectName);
			object->GetTransform()->SetScale(scale);
		};

	// 各ステージ画像と、カメラ一画面分の配置位置。
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

	for (const StageBackgroundData& stageData : stageBackgroundDataList)
	{
		GameObject* background = Add2DObject();
		if (background == nullptr)
		{
			continue;
		}

		background->SetName(stageData.objectName);
		background->GetTransform()->SetPosition(stageData.position);
		background->GetTransform()->SetScale({ 1920.0f, 1080.0f, 1.0f });

		auto& animator =
			background->AddComponent<AnimatorComponent>(background->GetRenderer());

		animator.SetRenderLayer(RenderLayer::BackGround);
		animator.SetSortOrder(0);
		animator.SetStatic(stageData.texturePath);
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
}

void GameScene::OnExit()
{
	context.soundManager.StopBGM();
	Scene::OnExit();
}
