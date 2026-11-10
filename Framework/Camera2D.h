#pragma once
#include <DirectXMath.h>

class Camera2D
{
public:
	void Set2DViewRange(float range);

	DirectX::XMFLOAT2 GetPosition() const { return position; }
	void SetPosition(const DirectX::XMFLOAT2& p) { position = p; }

	void Move(const DirectX::XMFLOAT2& vector)
	{
		DirectX::XMFLOAT2 newPos = {position.x + vector.x, position.y + vector.y};
		position = newPos;
	}

	DirectX::XMMATRIX GetView() const
	{
		return DirectX::XMMatrixLookAtLH(
			{position.x, position.y, -5.f},
			{position.x, position.y, 0.f},
			{0.f, 1.f, 0.f});
	}

private:
	DirectX::XMFLOAT2 position = {0.f, 0.f};
};
