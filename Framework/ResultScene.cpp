#include "ResultScene.h"

#include <format>

#include "Keyboard.h"
#include "SceneManager.h"
#include "TextRenderer.h"
#include "WindowSettings.h"

void ResultScene::OnUpdate(float deltaTime)
{
	elapsed += deltaTime;
	if (elapsed < inputDelay)
	{
		return;
	}

	if (context.keyboard.KeyIsTriggered(VK_SPACE) || context.keyboard.KeyIsTriggered(VK_RETURN))
	{
		context.sceneManager->RequestChange("Title");
	}
}

void ResultScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);
	text.DrawScreen("ゲームオーバー", { WIN_WIDTH * 0.5f, 150.f },
		TextColor::Yellow, 3.f, TextAlign::Center);

	text.DrawScreen(std::format("タワーの高さ: {:.2f}", context.globalContext.towerHeight),
		{ WIN_WIDTH * 0.5f, 200.f }, TextColor::Yellow, 1.f, TextAlign::Center);

	if (elapsed >= inputDelay)
	{
		text.DrawScreen("Space / Enter でタイトルへ", { WIN_WIDTH * 0.5f, 350.f },
			TextColor::White, 1.f, TextAlign::Center);
	}
}
