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
