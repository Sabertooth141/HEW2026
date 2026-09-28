#include "MeshComponent.h"

#include "Topology.h"
#include "TransformCBuffer.h"
#include "VertexBuffer.h"

MeshComponent::MeshComponent(Renderer& renderer, MeshData& meshData, TransformComponent& transformComp)
{
	AddBind(std::make_unique<VertexBuffer>(renderer, meshData.vertices));
	AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, meshData.indices));
	AddBind(std::make_unique<TransformCBuffer>(renderer, transformComp));

	if (!IsStaticInit())
	{
		AddStaticBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}
}

void MeshComponent::Update(float deltaTime)
{
	IComponent::Update(deltaTime);
}
