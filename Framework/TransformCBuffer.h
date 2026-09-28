#pragma once
#include <DirectXMath.h>

#include "Bindable.h"
#include "ConstantBuffer.h"
#include "TransformComponent.h"

struct Transforms
{
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

class TransformCBuffer : public Bindable
{
public:
	TransformCBuffer(Renderer& renderer, const TransformComponent& transformComp);

	void Bind(Renderer& renderer) override;

private:
	VertexConstantBuffer<Transforms> vCBuffer;
	const TransformComponent& transformComp;
};

