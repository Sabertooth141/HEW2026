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
	DirectX::XMMATRIX projectionMatrix;

	if (renderer.IsScreenSpaceEnabled())
	{
		viewMatrix = DirectX::XMMatrixIdentity();
		projectionMatrix = renderer.GetUIProj();
	}
	else
	{
		viewMatrix = renderer.GetView();
		projectionMatrix = renderer.GetProj();
	}

	transforms.model = DirectX::XMMatrixTranspose(transformComp.GetMatrix());
	transforms.view = DirectX::XMMatrixTranspose(viewMatrix);
	transforms.projection = DirectX::XMMatrixTranspose(projectionMatrix);

	vCBuffer.Update(renderer, transforms);
	vCBuffer.Bind(renderer);
}
