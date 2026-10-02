#pragma once
#include "Scene.h"

class ResultScene : public Scene
{
public:
	using Scene::Scene;

	void OnUpdate(float deltaTime) override;
	void OnDrawUI(TextRenderer& text) override;

private:
	// ignore input briefly so Space-mashing from gameplay doesn't skip the result
	static constexpr float inputDelay = 1.f;
	float elapsed = 0.f;
};

