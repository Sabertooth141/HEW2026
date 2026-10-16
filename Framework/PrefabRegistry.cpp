#include "PrefabRegistry.h"

#include "GameObject.h"
#include "EnemyController.h"
#include "PhysicsTest.h"

#include "BoxCollider2D.h"
#include "Rigidbody2DComponent.h"

void RegisterPrefabs()
{
	// enemy
	PrefabRegistry::Instance().Register("enemy", [](GameObject& object)
	{
		object.SetTag(ObjectTag::ENEMY);

		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Enemy);
		anim.SetStatic(L"../../assets/jinx.jpg");

		object.GetTransform()->SetScale({ 48.f, 48.f, 1.f });

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
