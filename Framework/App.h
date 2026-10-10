#pragma once
#include "Window.h"
#include <string>

#include "AnimationSystem.h"
#include "Camera2D.h"
#include "DebugRenderer.h"
#include "EditorUI.h"
#include "LightCBuffer.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "ScriptSystem.h"
#include "Timer.h"
#include "GameContext.h"
#include "ImGuiLayer.h"
#include "SceneManager.h"
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
	void DebugRender(Scene& scene);
	void DebugTextRender(Scene& scene, float deltaTime);

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
	SceneManager sceneManager;
	ImGuiLayer imGui;
	EditorUI editor;

	std::unique_ptr<LightCBuffer> lightCBuffer;

	float sensitivity = 0.004f;
};
