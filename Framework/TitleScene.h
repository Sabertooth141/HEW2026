#pragma once
#include "Scene.h"

class TitleScene : public Scene
{
public:
	using Scene::Scene;

	void OnEnter() override;
	void OnUpdate(float deltaTime) override;
	void OnDrawUI(TextRenderer& text) override;
	void OnExit() override;
};

