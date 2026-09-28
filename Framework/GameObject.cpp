#include "GameObject.h"

#include "AnimationSystem.h"
#include "App.h"
#include "ModelReader.h"
#include "Material.h"

GameObject::GameObject(const MaterialData& matData, const ModelReader& modelReader, GameContext& context,
                       const std::wstring& vsPath, const std::wstring& psPath)
{
	this->context = &context;
	scriptSystem = &context.scriptSys;
	animationSystem = &context.animationSys;
	renderSystem = &context.renderSys;
	renderer = &context.renderer;
	physicsSystem = &context.physicsSys;
	TransformComponent& transformComp = AddComponent<TransformComponent>();
	AddComponent<MaterialComponent>(*renderer, matData, vsPath, psPath);
	for (auto& meshData : modelReader.GetMeshes())
	{
		meshes.push_back(std::make_unique<MeshComponent>(*renderer, meshData, transformComp));
	}
}

GameObject::GameObject(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GameContext& context,
                       const std::wstring& vsPath, const std::wstring& psPath)
{
	this->context = &context;
	scriptSystem = &context.scriptSys;
	animationSystem = &context.animationSys;
	renderSystem = &context.renderSys;
	renderer = &context.renderer;
	physicsSystem = &context.physicsSys;
	TransformComponent& transformComp = AddComponent<TransformComponent>();
	AddComponent<MaterialComponent>(*renderer, MaterialData{}, vsPath, psPath);
	MeshData mesh = {vertices, indices};
	meshes.push_back(std::make_unique<MeshComponent>(*renderer, mesh, transformComp));
}

GameObject::GameObject(Renderer& renderer)
{
	TransformComponent& transformComp = AddComponent<TransformComponent>();
}

void GameObject::Update(float deltaTime)
{
	for (auto& component : components)
	{
		component.second->Update(deltaTime);
	}
}

TransformComponent* GameObject::GetTransform()
{
	return GetComponent<TransformComponent>();
}

std::vector<std::unique_ptr<MeshComponent>>& GameObject::GetMeshes()
{
	return meshes;
}

void GameObject::NotifyColliderEnter2D(const GameObject& other) const
{
	for (const auto& comp : components)
	{
		comp.second->OnCollisionEnter2D(other);
	}
}

void GameObject::NotifyColliderStay2D(const GameObject& other) const
{
	for (const auto& comp : components)
	{
		comp.second->OnCollisionStay2D(other);
	}
}

void GameObject::NotifyColliderLeave2D(const GameObject& other) const
{
	for (const auto& comp : components)
	{
		comp.second->OnCollisionExit2D(other);
	}
}
