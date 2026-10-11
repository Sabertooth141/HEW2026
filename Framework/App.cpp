#include "App.h"
#include "AnimatorComponent.h"
#include "CameraController.h"
#include "CollisionTests.h"
#include "GameObject.h"
#include "GameScene.h"
#include "Material.h"
#include "ModelReader.h"
#include "MonoBehavior.h"
#include "PhysicsTest.h"
#include "PlayerController.h"
#include "PrefabRegistry.h"
#include "ResultScene.h"
#include "SpriteRendererComponent.h"
#include "SpriteVertex.h"
#include "TitleScene.h"
#include "WindowSettings.h"
#include "HarpoonController.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine),
                                       wnd(WIN_WIDTH, WIN_HEIGHT, L"DXPractice"),
                                       renderer(wnd.GetRenderer()), debugRenderer(renderer),
                                       textRenderer(renderer, L"Consolas", 18),
                                       renderSystem(RenderSystem(renderer)),
                                       gameContext{
	                                       renderer, physicsSystem, scriptSystem, animationSystem, renderSystem, camera, wnd.keyboard, wnd.mouse
                                       },
                                       sceneManager(gameContext)
{
	gameContext.sceneManager = &sceneManager;
}

App::~App()
{
}

int App::Run()
{
	Init();

	timer.Mark();
	while (true)
	{
		if (const auto exitCode = Window::ProcessMessages())
		{
			return 0;
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

	sceneManager.Register<TitleScene>("Title");
	sceneManager.Register<GameScene>("Game");
	sceneManager.Register<ResultScene>("Result");
	sceneManager.RequestChange(sceneManager.IsRegistered(cmdLine) ? cmdLine : "Title");

	wnd.mouse.EnableRaw();
}

void App::Update(float deltaTime)
{
	Scene* scene = sceneManager.GetCurrScene();

	if (scene && !sceneManager.IsTransitioning())
	{
		scene->Update(deltaTime);
		scene->OnUpdate(deltaTime);

		bool levelUpSelecting = false;

		for (auto& go : scene->GetObjects())
		{
			auto* playerController =
				go->GetComponent<PlayerController>();

			if (playerController != nullptr &&
				playerController->IsLevelUpSelecting())
			{
				levelUpSelecting = true;
				break;
			}
		}

		if (!levelUpSelecting)
		{
			scriptSystem.Update(deltaTime);
			physicsSystem.Update(deltaTime);
			scriptSystem.LateUpdate(deltaTime);
			animationSystem.Update(deltaTime);
			scene->FlushPending();
		}
	}

	sceneManager.Update(deltaTime);
}

void App::HandleInput(float deltaTime)
{
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(0, 0, 0);
	textRenderer.Begin();

	debugRenderer.Begin();
	renderer.SetView(camera.GetView());

	if (Scene* scene = sceneManager.GetCurrScene())
	{
		GameScene* gameScene = dynamic_cast<GameScene*>(scene);

		if (gameScene != nullptr)
		{
			renderer.Set2DViewRange(gameScene->GetViewRange());
		}

		DebugRender(*scene);
		DebugTextRender(*scene, deltaTime);
		scene->OnDrawUI(textRenderer);
	}

	renderer.Set2DMode();
	renderSystem.Render();

	debugRenderer.Flush(renderer);
	textRenderer.Flush(renderer);

	renderer.EndFrame();
}

void App::DebugRender(Scene& scene)
{
	GameObject* player = nullptr;

	for (auto& go : scene.GetObjects())
	{
		if (auto* col = go->GetComponent<BoxCollider2D>())
		{
			debugRenderer.DrawBox(
				col->GetWorldOBB().GetCorners(),
				{ 0, 1, 0, 1 }
			);
		}

		if (go->GetComponent<PlayerController>() != nullptr)
		{
			player = go.get();
		}
	}

	if (player == nullptr)
	{
		return;
	}

	auto p = player->GetTransform()->GetPosition();

	for (auto& go : scene.GetObjects())
	{
		auto* controller =
			go->GetComponent<HarpoonController>();

		if (controller == nullptr || !controller->IsStuck())
		{
			continue;
		}

		GameObject* hammerEnemy =
			controller->GetStuckObject();

		if (hammerEnemy == nullptr)
		{
			continue;
		}

		auto e = hammerEnemy->GetTransform()->GetPosition();

		debugRenderer.DrawLine(
			{ p.x, p.y },
			{ e.x, e.y },
			{ 1, 0, 0, 1 }
		);
	}
}

void App::DebugTextRender(Scene& scene, float deltaTime)
{
	textRenderer.DebugLine("%.1f fps  (%.2f ms)", deltaTime > 0.f ? 1.f / deltaTime : 0.f, deltaTime * 1000.f);
	textRenderer.DebugLine("scene %s", sceneManager.GetCurrSceneName().c_str());
	textRenderer.DebugLine("objects %zu", scene.GetObjects().size());
	textRenderer.DebugLine(TextColor::Cyan, "cam y %.0f", camera.GetPosition().y);
}
