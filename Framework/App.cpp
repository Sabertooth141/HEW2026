#include "App.h"
#include "AnimatorComponent.h"
#include "CameraController.h"
#include "CollisionTests.h"
#include "GameObject.h"
#include "Material.h"
#include "ModelReader.h"
#include "MonoBehavior.h"
#include "PhysicsTest.h"
#include "PlayerController.h"
#include "PrefabRegistry.h"
#include "SpriteRendererComponent.h"
#include "SpriteVertex.h"
#include "WindowSettings.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine),
                                       wnd(WIN_WIDTH, WIN_HEIGHT, L"DXPractice"),
                                       renderer(wnd.GetRenderer()), debugRenderer(renderer),
                                       textRenderer(renderer, L"Consolas", 18),
                                       renderSystem(RenderSystem(renderer)),
                                       gameContext{
	                                       renderer, physicsSystem, scriptSystem, animationSystem, renderSystem, camera
                                       },
                                       scene(gameContext)
{
}

App::~App()
{
}

int App::Run()
{
	Init();

	gameContext.gameScene = &scene;
	timer.Mark();
	while (true)
	{
		if (const auto exitCode = Window::ProcessMessages())
		{
			return *exitCode;
		}

		const float deltaTime = timer.Mark();
		HandleInput(deltaTime);
		Update(deltaTime);
		Draw(deltaTime);

		wnd.keyboard.EndFrame();
	}
}

void App::Init()
{
	RegisterPrefabs();
	RegisterCollisionTests();
	// cube
	//std::unique_ptr<ModelReader> model = std::make_unique<ModelReader>(renderer, "../../assets/testCube.fbx");

	//MaterialData matData = {};
	//matData.color = DirectX::XMFLOAT3(0.3f, 0.4f, 1.0f);
	//matData.specularIntensity = 0.5f;
	//matData.specularPower = 5.0f;
	//std::unique_ptr<GameObject> cube = std::make_unique<GameObject>(renderer, matData, *model);
	//cube->GetTransform()->SetRotation(rotation);

	//gameObjects.push_back(std::move(cube));

	// player
	MeshData quad = MakeSpriteQuad();
	playerObj = scene.Add2DObject();

	playerObj->GetTransform()->SetPosition({-128, -100, 1});

	// physics
	Rigidbody2DComponent* playerRb = &playerObj->AddComponent<Rigidbody2DComponent>(*playerObj->GetTransform(), 1.0f);
	BoxCollider2D* col = &playerObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(1, 1), DirectX::XMFLOAT2(0, 0), true,
	                                                             *playerObj->GetTransform());
	playerRb->SetFreezeRotation(true);
	playerRb->SetGravity(0.f);

	// animation
	playerObj->AddComponent<AnimatorComponent>(renderer);
	playerObj->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Player);
	playerObj->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	playerObj->GetComponent<AnimatorComponent>()->AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png",
	                                                           L"../../assets/PlayerCharacter.json");
	playerObj->GetComponent<AnimatorComponent>()->AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
	                                                           L"../../assets/PlayerCharacterMove.json");
	playerObj->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharIdle");

	// script
	playerObj->AddComponent<PlayerController>();
	playerObj->GetComponent<PlayerController>()->SetInput(wnd.keyboard, wnd.mouse);
	playerObj->AddComponent<CameraController>();


	//// layer test
	//auto backGround = std::make_unique<GameObject>(quad.vertices, quad.indices, gameContext);

	//backGround->GetTransform()->SetPosition({0, 0, 1});

	//backGround->AddComponent<AnimatorComponent>(renderer);
	//backGround->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::BackGround);
	//backGround->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	//backGround->GetComponent<AnimatorComponent>()->SetStatic(L"../../assets/bgTest.jpg");

	// ground
	// physics test
	MeshData groundQuad = MakeSpriteQuad();
	auto groundObj = scene.Add2DObject();

	groundObj->GetTransform()->SetPosition({0, -200, 1});

	groundObj->AddComponent<AnimatorComponent>(renderer);
	groundObj->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Default);
	groundObj->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	groundObj->GetComponent<AnimatorComponent>()->SetStatic(L"../../assets/bgTest.jpg");
	groundObj->GetTransform()->SetScale({1000, 100, 1});

	groundObj->AddComponent<PhysicsTest>();

	// physics
	Rigidbody2DComponent* groundRb = &groundObj->AddComponent<Rigidbody2DComponent>(*groundObj->GetTransform(), 1.0f);
	BoxCollider2D* groundCol = &groundObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f),
	                                                                   DirectX::XMFLOAT2(0, 0),
	                                                                   false, *groundObj->GetTransform());
	groundRb->SetIsStatic(true);

	playerObj->GetComponent<CameraController>()->SetGroundTop(groundObj->GetTransform()->GetPosition().y);

	//block->GetComponent<Rigidbody2DComponent>()->SetFreezeRotation(true);

	wnd.mouse.EnableRaw();
}

void App::Update(float deltaTime)
{
	if (!gameRunning)
	{
		return;
	}

	textRenderer.Begin();
	HandleInput(deltaTime);

	scene.Update(deltaTime);

	// systems
	scriptSystem.Update(deltaTime);
	physicsSystem.Update(deltaTime);
	scriptSystem.LateUpdate(deltaTime);
	animationSystem.Update(deltaTime);

	// at the end for GC
	scene.FlushPending();

	playTimer -= deltaTime;
	if (playTimer <= 0.f)
	{
		gameRunning = false;
	}
}

void App::HandleInput(float deltaTime)
{
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(0, 0, 0);
	textRenderer.Begin();

	if (gameRunning)
	{
		DebugRender(deltaTime);
		DebugTextRender(deltaTime);
		debugRenderer.Flush(renderer);
	}
	else
	{
		textRenderer.DrawScreen("ゲームオーバー", {WIN_WIDTH * 0.5f, 150.f},
		                        TextColor::Yellow, 3.f, TextAlign::Center);
		CameraController* camera = playerObj->GetComponent<CameraController>();

		textRenderer.DrawScreen(
			std::format("タワーの高さ: {:.2f}", camera->GetStackTopY() - camera->GetGroundTop()),
			{WIN_WIDTH * 0.5f, 200.f}, TextColor::Yellow, 1.f, TextAlign::Center);
	}

	textRenderer.Flush(renderer);
	renderer.EndFrame();
}

void App::DebugRender(float deltaTime)
{
	if (lightCBuffer != nullptr)
	{
		lightCBuffer->Bind(renderer);
	}

	renderer.SetView(camera.GetView());
	renderer.Set2DMode();

	renderSystem.Render();
	debugRenderer.Begin();
	for (auto& go : scene.GetObjects())
		if (auto* col = go->GetComponent<BoxCollider2D>())
			debugRenderer.DrawBox(col->GetWorldOBB().GetCorners(), {0, 1, 0, 1});
}

void App::DebugTextRender(float deltaTime)
{
	textRenderer.DebugLine("%.1f fps  (%.2f ms)", deltaTime > 0.f ? 1.f / deltaTime : 0.f, deltaTime * 1000.f);
	textRenderer.DebugLine("objects %zu", scene.GetObjects().size());
	textRenderer.DebugLine(TextColor::Cyan, "cam y %.0f", camera.GetPosition().y);

	CameraController* camera = playerObj->GetComponent<CameraController>();

	textRenderer.DrawScreen(std::format("タワーの高さ: {:.2f}", camera->GetStackTopY() + camera->GetGroundTop()),
	                        {WIN_WIDTH * 0.5f, 24.f}, TextColor::Yellow, 1.f, TextAlign::Center);

	textRenderer.DrawScreen(std::format("残り時間: {:.2f}", playTimer), {WIN_WIDTH * 0.5f, 40.f}, TextColor::Yellow, 1.f,
	                        TextAlign::Center);
}
