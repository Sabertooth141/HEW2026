#include "PrefabRegistry.h"

#include "GameObject.h"
#include "EnemyController.h"
#include "PhysicsTest.h"

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
		object.AddComponent<EnemyController>();
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
}
