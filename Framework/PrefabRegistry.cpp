#include "PrefabRegistry.h"

#include "GameObject.h"
#include "EnemyController.h"
#include "PhysicsTest.h"

#include "BoxCollider2D.h"
#include "CameraController.h"
#include "PlayerController.h"
#include "Rigidbody2DComponent.h"
#include "TextLabel.h"

void RegisterPrefabs()
{
	// player
	PrefabRegistry::Instance().Register("player", [](GameObject& object)
	{
		object.SetTag(ObjectTag::PLAYER);

		// physics
		auto& playerRb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f);
		playerRb.SetFreezeRotation(true);
		playerRb.SetGravity(0.f);
		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(1.f, 1.f), DirectX::XMFLOAT2(0.f, 0.f), true, *object.GetTransform());

		// animation
		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Player);
		anim.SetSortOrder(0);
		anim.AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png", L"../../assets/PlayerCharacter.json");
		anim.AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
		                  L"../../assets/PlayerCharacterMove.json");
		anim.SetCurrAnimation("CharIdle");

		// script
		auto& controller = object.AddComponent<PlayerController>();
		controller.SetInput(object.GetContext().keyboard, object.GetContext().mouse);
		object.AddComponent<CameraController>();

		auto& label = object.AddComponent<TextLabel>();
		label.posOffset = { 00.f, 00.f };
	});


	// enemy
	PrefabRegistry::Instance().Register("enemy", [](GameObject& object)
	{
		object.SetTag(ObjectTag::ENEMY);

		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Enemy);
		anim.SetStatic(L"../../assets/jinx.jpg");

		object.GetTransform()->SetScale({48.f, 48.f, 1.f});

		auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f);
		rb.SetGravity(0.f);
		rb.SetFreezeRotation(true);
		rb.SetIsStatic(false);

		// 当たり判定を画像サイズにあわせる
		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(0.5f, 0.5f),
			DirectX::XMFLOAT2(0.f, 0.f),
			false,
			*object.GetTransform());

		auto& enemy = object.AddComponent<EnemyController>();
		enemy.SetStatus(30, 10, 60.f);
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
}
