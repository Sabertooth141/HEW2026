#pragma once
#include "Window.h"
#include <string>

#include "AnimationSystem.h"
#include "Camera2D.h"
#include "DebugRenderer.h"
#include "LightCBuffer.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "Scene.h"
#include "ScriptSystem.h"
#include "Timer.h"
#include "GameContext.h"
#include "TextRenderer.h"

class GameObject;
class ModelReader;

class App
{
public:
	App(const std::string& cmdLine = "");
	~App();

	int Run();

private:
	void Init();
	void Update(float deltaTime);
	void HandleInput(float deltaTime);
	void Draw(float deltaTime);
	void DebugRender(float deltaTime);
	void DebugTextRender(float deltaTime);

private:
	DirectX::XMFLOAT3 rotation = {};

	std::string cmdLine;

	Window wnd;
	Renderer renderer;
	DebugRenderer debugRenderer;
	TextRenderer textRenderer;

	Timer timer;
	ScriptSystem scriptSystem;
	AnimationSystem animationSystem;
	RenderSystem renderSystem;
	PhysicsSystem physicsSystem;
	Camera2D camera;

	GameContext gameContext;
	Scene scene;

	std::unique_ptr<LightCBuffer> lightCBuffer;

	bool gameRunning = true;

	float sensitivity = 0.004f;

	float playTime = 25.f;
	float playTimer = playTime;

	GameObject* playerObj = nullptr;
};

