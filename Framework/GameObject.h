#pragma once
#include <typeindex>
#include <unordered_map>

#include "AnimationSystem.h"
#include "BoxCollider2D.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MonoBehavior.h"
#include "PhysicsSystem.h"
#include "ScriptSystem.h"
#include "RenderSystem.h"
#include "GameContext.h"

class Rigidbody2DComponent;
class PhysicsSystem;
struct GameContext;
class ModelReader;
class Renderer;

enum class ObjectTag : uint8_t
{
	Default,
	Enemy,
	Player,
	Block,
	Ground
};

class GameObject
{
public:
	// for 3d
	GameObject(const MaterialData& matData, const ModelReader& modelReader, GameContext& context,
	           const std::wstring& vsPath = L"VertexShader.cso", const std::wstring& psPath = L"PixelShader.cso");
	// for 2d
	GameObject(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GameContext& context,
	           const std::wstring& vsPath = L"SpriteVertexShader.cso", const std::wstring& psPath = L"SpritePixelShader.cso");
	GameObject(Renderer& renderer);

	void Update(float deltaTime);

	template <typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		std::unique_ptr<T> comp = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *comp;
		ref.owner = this;

		if constexpr (std::is_base_of_v<MonoBehavior, T>)
		{
			scriptSystem->RegisterScript(&ref);
		}
		else if constexpr (std::is_base_of_v<AnimatorComponent, T>)
		{
			animationSystem->Register(&ref);
			renderSystem->Register(&ref);
		}
		else if constexpr (std::is_base_of_v<Rigidbody2DComponent, T>)
		{
			physicsSystem->AttachRBToEntry(this, &ref);
		}
		else if constexpr (std::is_base_of_v<Collider2D, T>)
		{
			physicsSystem->AttachColliderToEntry(this, &ref);
		}

		components[typeid(T)] = std::move(comp);
		return ref;
	}

	template <typename T>
	T* GetComponent()
	{
		auto res = components.find(typeid(T));
		if (res != components.end())
		{
			return static_cast<T*>(res->second.get());
		}
		return nullptr;
	}

	void NotifyColliderEnter2D(const GameObject& other) const;
	void NotifyColliderStay2D(const GameObject& other) const;
	void NotifyColliderLeave2D(const GameObject& other) const;

	Renderer& GetRenderer() const
	{
		return *renderer;
	}

	GameContext& GetContext() const
	{
		return *context;
	}

	TransformComponent* GetTransform();
	std::vector<std::unique_ptr<MeshComponent>>& GetMeshes();

	ObjectTag GetTag() const
	{
		return tag;
	}

	void SetTag(ObjectTag inTag)
	{
		tag = inTag;
	}

private:
	ObjectTag tag = ObjectTag::Default;
	std::unordered_map<std::type_index, std::unique_ptr<IComponent>> components;
	std::vector<std::unique_ptr<MeshComponent>> meshes;

	GameContext* context;
	Renderer* renderer = nullptr;
	ScriptSystem* scriptSystem = nullptr;
	AnimationSystem* animationSystem = nullptr;
	RenderSystem* renderSystem = nullptr;
	PhysicsSystem* physicsSystem = nullptr;
};
