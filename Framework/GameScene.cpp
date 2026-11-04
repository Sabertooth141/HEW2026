#include "GameScene.h"

#include <algorithm>
#include <format>

#include "CameraController.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "TextRenderer.h"
#include "WindowSettings.h"
#include "RetryTransitionComponent.h"

void GameScene::OnEnter()
{
	Scene::OnEnter();
	//2Dプラットフォーム
	Instantiate("platform_layout", { 0.0f,0.0f,1.0f });

	// enemies (prefab)
	Instantiate("enemy", {200.f, -100.f, 1.f});
	Instantiate("enemy", {-300.f, 150.f, 1.f});

	//チェックポイント
	Instantiate("checkpoint_layout", { 0.0f,0.0f,1.0f });
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
