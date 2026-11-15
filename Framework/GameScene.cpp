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

	// ステージ全体の背景画像を生成する。
	// 背景なので、当たり判定やRigidbodyは付けない。
	GameObject* background = Add2DObject();

	if (background != nullptr)
	{
		background->SetName("StageBackground");

		auto& animator =
			background->AddComponent<AnimatorComponent>(background->GetRenderer());

		animator.SetRenderLayer(RenderLayer::BackGround);
		animator.SetSortOrder(0);
		animator.SetStatic(L"../../assets/haikei.png");

		// 5画面分の幅、2画面分の高さに合わせて配置する。
		// ステージ1の画面中心が (0, 0) の場合の座標。
		background->GetTransform()->SetPosition({ 3840.0f, -540.0f, 1.0f });
		background->GetTransform()->SetScale({ 9600.0f, 2160.0f, 1.0f });
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
