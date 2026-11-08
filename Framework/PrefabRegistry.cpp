#include "PrefabRegistry.h"

#include "GameObject.h"
#include "PhysicsTest.h"

#include "BoxCollider2D.h"
#include "CameraController.h"
#include "PlayerController.h"
#include "Rigidbody2DComponent.h"
#include "TextLabel.h"
#include "PlatformLayoutComponent.h"
#include "AnimatorComponent.h"
#include "CheckpointComponent.h"
#include "CheckpointLayoutComponent.h"
#include "RetryTransitionComponent.h"
#include "SpeedBoostComponent.h"

void RegisterPrefabs()
{
	// player
	PrefabRegistry::Instance().Register("player", [](GameObject& object)
	{
		object.SetTag(ObjectTag::PLAYER);

		// physics
		auto& playerRb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f);
		playerRb.SetFreezeRotation(true);
		playerRb.SetGravity(800.f);
		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(1.f, 1.f), DirectX::XMFLOAT2(0.f, 0.f), false, *object.GetTransform());

		// animation
		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Player);
		anim.SetSortOrder(0);
		anim.AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png", L"../../assets/PlayerCharacter.json");
		anim.AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
		                  L"../../assets/PlayerCharacterMove.json");
		anim.SetCurrAnimation("CharIdle");

		//加速
		object.AddComponent<SpeedBoostComponent>();

		// script
		auto& controller = object.AddComponent<PlayerController>();
		controller.SetInput(object.GetContext().keyboard, object.GetContext().mouse);
		object.AddComponent<CameraController>();

		//トランジション
		object.AddComponent<RetryTransitionComponent>();

		auto& label = object.AddComponent<TextLabel>();
		label.posOffset = { 10.f, 0.f };
		label.isDrawingScreen = true;
	});

	// block (physics sample)
	PrefabRegistry::Instance().Register("block", [](GameObject& object)
	{
		object.SetTag(ObjectTag::BLOCK);

		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Default);
		anim.SetSortOrder(0);
		anim.SetStatic(L"../../assets/jinx.jpg");

		object.GetTransform()->SetScale(0.05f);

		auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
		rb.SetGravity(500.f);

		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(0.5f, 0.5f),
			DirectX::XMFLOAT2(0.f, 0.f),
			false,
			*object.GetTransform());

		object.AddComponent<PhysicsTest>();
	});

	//プラットフォーム
	PrefabRegistry::Instance().Register("platform_layout", [](GameObject& object)
		{
			object.AddComponent<PlatformLayoutComponent>();
		});

	//チェックポイント
	PrefabRegistry::Instance().Register("checkpoint", [](GameObject& object)
		{
			object.SetTag(ObjectTag::CHECKPOINT);

			//仮の表示画像
			auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
			anim.SetRenderLayer(RenderLayer::Default);
			anim.SetSortOrder(1);
			anim.SetStatic(L"../../assets/flag.png");

			//表示サイズと判定の大きさ
			object.GetTransform()->SetScale({ 72.0f,72.0f,1.0f });

			auto& checkpointRb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
			checkpointRb.SetGravity(0.0f);
			checkpointRb.SetIsStatic(true);

			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				true,
				*object.GetTransform());

			object.AddComponent<CheckpointComponent>();
		});

	PrefabRegistry::Instance().Register("checkpoint_layout", [](GameObject& object)
		{
			object.AddComponent<CheckpointLayoutComponent>();
		});

	//針
	PrefabRegistry::Instance().Register("spike", [](GameObject& object)
		{
			object.SetTag(ObjectTag::SPIKE);
			object.GetTransform()->SetScale({ 64.f, 64.f, 1.f });
		
			auto& spikeRb = object.AddComponent<Rigidbody2DComponent>(
				*object.GetTransform(), 1.0f);

			spikeRb.SetGravity(0.0f);
			spikeRb.SetIsStatic(true);

			// 当たり判定
			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				true,
				*object.GetTransform());
		});
}
