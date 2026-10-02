#pragma once
#include "Scene.h"

class GameScene : public Scene
{
public:
	using Scene::Scene;

	void OnEnter() override;
	void OnUpdate(float deltaTime) override;
	void OnDrawUI(TextRenderer& text) override;

private:
	float GetTowerHeight() const;

private:
	GameObject* playerObj = nullptr;

	float playTime = 25.f;
	float playTimer = playTime;
	bool finished = false;
};