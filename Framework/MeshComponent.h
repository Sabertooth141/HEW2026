#pragma once
#include <memory>
#include <vector>

#include "IComponent.h"
#include "DrawableBase.h"
#include "MeshData.h"
#include "TransformComponent.h"

/**
 * handles vertices, indices, binds VB IB and topology on construct
 * takes component class as interface, drawableBase as base class
 */
class MeshComponent : public IComponent, public DrawableBase<MeshComponent>
{
public:
	MeshComponent(Renderer& renderer, MeshData& meshData, TransformComponent& transformComp);
	void Update(float deltaTime) override;
};

