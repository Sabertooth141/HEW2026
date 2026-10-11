#include "TransformCBuffer.h"

TransformCBuffer::TransformCBuffer(Renderer& renderer, const TransformComponent& transformComp) :
	vCBuffer(renderer, Transforms{}, 0),
	transformComp(transformComp)
{
}

void TransformCBuffer::Bind(Renderer& renderer)
{
	Transforms transforms;

	DirectX::XMMATRIX viewMatrix;

	if (renderer.IsScreenSpaceEnabled())
	{
		viewMatrix = DirectX::XMMatrixIdentity();
	}
	else
	{
		viewMatrix = renderer.GetView();
	}

	transforms.model = DirectX::XMMatrixTranspose(transformComp.GetMatrix());
	transforms.view = DirectX::XMMatrixTranspose(viewMatrix);
	transforms.projection = DirectX::XMMatrixTranspose(renderer.GetProj());

	vCBuffer.Update(renderer, transforms);
	vCBuffer.Bind(renderer);
}
