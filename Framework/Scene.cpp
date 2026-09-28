#include "Scene.h"
#include "PrefabRegistry.h"
#include "SpriteVertex.h"
#include "App.h"

GameObject* Scene::Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos)
{
	const PrefabFn* prefabFunction = PrefabRegistry::Instance().Find(prefab);
	if (!prefabFunction)
	{
		assert(false && "Scene::Instantiate: unknown prefab");
		return nullptr;
	}

	MeshData quad = MakeSpriteQuad();
	auto object = std::make_unique<GameObject>(
		quad.vertices, quad.indices, context);

	object->GetTransform()->SetPosition(pos);
	(*prefabFunction)(*object);

	GameObject* raw = object.get();
	pendingSpawn.push_back(std::move(object));
	return raw;
}

GameObject* Scene::Add2DObject()
{
	MeshData quad = MakeSpriteQuad();
	auto object = std::make_unique<GameObject>(quad.vertices, quad.indices, context);
	objects.push_back(std::move(object));

	return objects.back().get();
}

void Scene::Destroy(GameObject* object)
{
	if (!object)
	{
		return;
	}

	if (std::ranges::find(pendingDestroy, object) != pendingDestroy.end())
	{
		return;
	}

	pendingDestroy.push_back(object);
}

void Scene::Update(float deltaTime)
{
	for (auto& object : objects)
		object->Update(deltaTime);
}

void Scene::FlushPending()
{
	for (GameObject* object : pendingDestroy)
	{
		context.physicsSys.Unregister(object);
		context.scriptSys.Unregister(object);

		if (auto* anim = object->GetComponent<AnimatorComponent>())
		{
			context.animationSys.Unregister(anim);
		}

		context.renderSys.Unregister(object);

		std::erase_if(objects, [this](const std::unique_ptr<GameObject>& p)
		{
			return std::ranges::find(pendingDestroy, p.get()) != pendingDestroy.end();
		});
	}
	pendingDestroy.clear();

	for (auto& object : pendingSpawn)
	{
		objects.push_back(std::move(object));
	}
	pendingSpawn.clear();
}
