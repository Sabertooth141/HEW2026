#pragma once
#include "Scene.h"

// サンプル用のゲームシーン
// プレイヤーと敵を配置し、プレイヤーのHPが0になったらResultへ移る
class GameScene : public Scene
{
public:
	using Scene::Scene;

	void OnEnter() override;
	void OnUpdate(float deltaTime) override;
	void OnDrawUI(TextRenderer& text) override;

private:
	GameObject* playerObj = nullptr;

	float playTimer = 0.f;
	bool finished = false;
};
