#pragma once
#include "Scene.h"
#include "PlayerStatus.h"

#include <vector>

struct LevelUpChoiceRect
{
	float centerX;
	float centerY;
	float width;
	float height;
};

class GameScene : public Scene
{
public:
	using Scene::Scene;

	void OnEnter() override;
	void OnUpdate(float deltaTime) override;
	void OnDrawUI(TextRenderer& text) override;

	float GetViewRange() const;

	LevelUpChoiceRect GetLevelUpChoiceRect(size_t index) const;
	bool IsLevelUpSelecting() const;

private:
	float GetTowerHeight() const;



	// レベルアップ関連
	void HandleLevelUpInput();

private:
	GameObject* playerObj = nullptr;
	GameObject* levelUpChoiceObjects[3] = {};

	float playTime = 25.f;
	float playTimer = playTime;
	bool finished = false;
};