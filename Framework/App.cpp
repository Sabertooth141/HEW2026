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
	imGui.Init(wnd.GetHwnd(), renderer.GetDevice().Get(), renderer.GetContext().Get());
}

void App::Update(float deltaTime)
{
	Scene* scene = sceneManager.GetCurrScene();
	if (scene && !sceneManager.IsTransitioning())
	{
		if (editor.ShouldTick())
		{
			auto* gameScene = dynamic_cast<GameScene*>(scene);

			if (gameScene && gameScene->IsLevelUpSelecting())
			{
				// 選択中は選択入力だけ処理する
				scene->OnUpdate(deltaTime);
			}
			else
			{
				scene->Update(deltaTime);
				scene->OnUpdate(deltaTime);
				scriptSystem.Update(deltaTime);
				physicsSystem.Update(deltaTime);
				scriptSystem.LateUpdate(deltaTime);
				animationSystem.Update(deltaTime);
			}
		}
		scene->FlushPending();
	}
	sceneManager.Update(deltaTime); // switch happens here
}

void App::HandleInput(float deltaTime)
{
	if (wnd.keyboard.KeyIsTriggered(VK_F1))
	{
		editor.ToggleVisible();
	}
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(0, 0, 0);
	imGui.BeginFrame();
	textRenderer.Begin();

	debugRenderer.Begin();
	renderer.SetView(camera.GetView());
	if (auto* gameScene =
		dynamic_cast<GameScene*>(sceneManager.GetCurrScene()))
	{
		renderer.Set2DViewRange(gameScene->GetViewRange());
	}
	renderer.Set2DMode();
	renderSystem.Render();

	if (Scene* scene = sceneManager.GetCurrScene())
	{
		DebugRender(*scene);
		DebugTextRender(*scene, deltaTime);
		scene->OnDrawUI(textRenderer);
	}

	editor.Draw(sceneManager, camera);

	debugRenderer.Flush(renderer);
	textRenderer.Flush(renderer);

	imGui.EndFrame();
	renderer.EndFrame();
}

void App::DebugRender(Scene& scene)
{
	GameObject* player = nullptr;

	// プレイヤーを探す
	for (auto& go : scene.GetObjects())
	{
		if (go->GetComponent<PlayerController>() != nullptr)
		{
			player = go.get();
			break;
		}
	}

	// プレイヤーがいなければ終了
	if (player == nullptr)
	{
		return;
	}

	auto playerPos = player->GetTransform()->GetPosition();

	for (auto& go : scene.GetObjects())
	{
		// 当たり判定の枠を表示
		if (auto* col = go->GetComponent<BoxCollider2D>())
		{
			debugRenderer.DrawBox(
				col->GetWorldOBB().GetCorners(),
				{ 0, 1, 0, 1 });
		}

		// ハンマー状態の銛を探す
		auto* harpoon = go->GetComponent<HarpoonController>();

		if (harpoon != nullptr && harpoon->IsStuck())
		{
			auto hammerPos = go->GetTransform()->GetPosition();

			// プレイヤーとハンマー状態の敵を赤い線でつなぐ
			debugRenderer.DrawLine(
				{ playerPos.x, playerPos.y },
				{ hammerPos.x, hammerPos.y },
				{ 1, 0, 0, 1 });
		}
	}
}
void App::DebugTextRender(Scene& scene, float deltaTime)
{
	textRenderer.DebugLine("%.1f fps  (%.2f ms)", deltaTime > 0.f ? 1.f / deltaTime : 0.f, deltaTime * 1000.f);
	textRenderer.DebugLine("scene %s", sceneManager.GetCurrSceneName().c_str());
	textRenderer.DebugLine("objects %zu", scene.GetObjects().size());
	textRenderer.DebugLine(TextColor::Cyan, "cam y %.0f", camera.GetPosition().y);
}

