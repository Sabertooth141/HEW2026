#pragma once
#include <DirectXMath.h>
#include <array>
#include <cmath>

#include "Collider2D.h"

class TransformComponent;

struct OBB
{
	DirectX::XMFLOAT2 center;
	DirectX::XMFLOAT2 halfExtents;
	float rotation; // in radians

	std::array<DirectX::XMFLOAT2, 2> GetAxes() const
	{
		const float c = std::cos(rotation);
		const float s = std::sin(rotation);

		return {DirectX::XMFLOAT2{c, s}, DirectX::XMFLOAT2{-s, c}};
	}

	std::array<DirectX::XMFLOAT2, 4> GetCorners() const
	{
		const float c = std::cos(rotation);
		const float s = std::sin(rotation);

		const std::array<DirectX::XMFLOAT2, 4> localCorners =
		{
			DirectX::XMFLOAT2{-halfExtents.x, -halfExtents.y},
			DirectX::XMFLOAT2{halfExtents.x, -halfExtents.y},
			DirectX::XMFLOAT2{halfExtents.x, halfExtents.y},
			DirectX::XMFLOAT2{-halfExtents.x, halfExtents.y}
		};

		std::array<DirectX::XMFLOAT2, 4> worldCorners;
		for (int i = 0; i < 4; i++)
		{
			worldCorners[i] =
			{
				center.x + localCorners[i].x * c - localCorners[i].y * s,
				center.y + localCorners[i].x * s + localCorners[i].y * c
			};
		}

		return worldCorners;
	}
};

class BoxCollider2D : public Collider2D
{
public:
	BoxCollider2D(DirectX::XMFLOAT2 inHalfExtents, DirectX::XMFLOAT2 inOffset, bool inIsTrigger,
	              TransformComponent& inTransformComp);

	ColliderType GetType() const override
	{
		return ColliderType::Box;
	}

	AABB GetWorldAABB() const override;
	OBB GetWorldOBB() const;
	float ComputeInertia(const float mass) const override;

	DirectX::XMFLOAT2 GetHalfExtents() const
	{
		return halfExtents;
	}

	DirectX::XMFLOAT2 GetOffset() const
	{
		return offset;
	}

private:
	TransformComponent* transformComp = nullptr;
	DirectX::XMFLOAT2 halfExtents;
	DirectX::XMFLOAT2 offset;
};
