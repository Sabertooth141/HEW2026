#include "TitleScene.h"

#include "Keyboard.h"
#include "SceneManager.h"
#include "TextRenderer.h"
#include "WindowSettings.h"

void TitleScene::OnEnter()
{
	Scene::OnEnter();
}

void TitleScene::OnUpdate(float deltaTime)
{
	if (context.keyboard.KeyIsTriggered(VK_RETURN))
	{
		context.sceneManager->RequestChange("Game");
	}
}

void TitleScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);
	text.DrawScreen("TITLE", { WIN_WIDTH * 0.5f, 200.f }, TextColor::Yellow, 3.f, TextAlign::Center);
	text.DrawScreen("ENTER: スタート", {WIN_WIDTH * 0.5f, 300.f}, TextColor::White, 1.f, TextAlign::Center);
}

void TitleScene::OnExit()
{
	Scene::OnExit();
}
