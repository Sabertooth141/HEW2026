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

	// enemies (prefab)
	Instantiate("enemy", {200.f, -100.f, 1.f});
	Instantiate("enemy", {-300.f, 150.f, 1.f});
}

void GameScene::OnUpdate(float deltaTime)
{
}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);
}
