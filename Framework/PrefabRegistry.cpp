#include "PrefabRegistry.h"

#include "GameObject.h"
#include "EnemyController.h"
#include "PhysicsTest.h"

#include "BoxCollider2D.h"
#include "Rigidbody2DComponent.h"

void RegisterPrefabs()
{
	//Enemy
	PrefabRegistry::Instance().Register("enemy", [](GameObject& object)
	{
		object.SetTag(ObjectTag::Enemy);

		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());

		anim.SetRenderLayer(RenderLayer::Enemy);
		anim.SetStatic(L"../../assets/jinx.jpg");

		object.GetTransform()->SetScale({ 48.f, 48.f, 1.f });

		auto& enemyBody = object.AddComponent<Rigidbody2DComponent>(
			*object.GetTransform(), 1.f);

		enemyBody.SetGravity(0.f);
		enemyBody.SetFreezeRotation(true);
		enemyBody.SetIsStatic(false);

		//当たり判定を画像サイズにあわせる
		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(0.5f, 0.5f),
			DirectX::XMFLOAT2(0.f, 0.f),
			true,
			*object.GetTransform());

		auto& enemy = object.AddComponent<EnemyController>();

		// デフォルトのステータスを設定
		enemy.SetStatus(30, 10, 60.f, 5);

		enemy.knockbackSpeed = 240.f;
		enemy.knockbackDuration = 0.2f;
	});

	// blocks
	PrefabRegistry::Instance().Register("block", [](GameObject& object)
	{
		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Default);
		anim.SetSortOrder(0);
		anim.SetStatic(L"../../assets/jinx.jpg");

		object.GetTransform()->SetScale(0.05f);

		auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
		rb.SetGravity(500.f);

		object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f), DirectX::XMFLOAT2(0.f, 0.f), false, *object.GetTransform());

		object.AddComponent<PhysicsTest>();
		object.SetTag(ObjectTag::Block);
	});

	//Experience
	PrefabRegistry::Instance().Register(
		"experience", [](GameObject& object)
		{
			object.SetTag(ObjectTag::Experience);

			auto& anim =
				object.AddComponent<AnimatorComponent>(object.GetRenderer());

			anim.SetRenderLayer(RenderLayer::Default);

			anim.SetStatic(L"../../assets/jinx.jpg");

			object.GetTransform()->SetScale({ 12.f, 12.f, 1.f });
		});
}
