#include "BoxCollider2D.h"
#include "TransformComponent.h"

BoxCollider2D::BoxCollider2D(const DirectX::XMFLOAT2 inHalfExtents, const DirectX::XMFLOAT2 inOffset,
                             const bool inIsTrigger,
                             TransformComponent& inTransformComp) :
	transformComp(&inTransformComp),
	halfExtents(inHalfExtents),
	offset(inOffset)
{
	isTrigger = inIsTrigger;
}

AABB BoxCollider2D::GetWorldAABB() const
{
	const auto corners = GetWorldOBB().GetCorners();

	DirectX::XMFLOAT2 min = corners[0];
	DirectX::XMFLOAT2 max = corners[0];
	for (int i = 1; i < 4; i++)
	{
		min.x = std::min(min.x, corners[i].x);
		min.y = std::min(min.y, corners[i].y);
		max.x = std::max(max.x, corners[i].x);
		max.y = std::max(max.y, corners[i].y);
	}

	return AABB{.min = min, .max = max};
}

OBB BoxCollider2D::GetWorldOBB() const
{
	const DirectX::XMFLOAT3 position = transformComp->GetPosition();
	const DirectX::XMFLOAT3 scale = transformComp->GetScale();
	const DirectX::XMFLOAT3 rotation = transformComp->GetRotation();

	// for 2d plane rotation on z axis
	const float c = std::cos(rotation.z);
	const float s = std::sin(rotation.z);

	// sync offset rotation for collider
	const DirectX::XMFLOAT2 rotatedOffset =
	{
		offset.x * c - offset.y * s,
		offset.x * s + offset.y * c
	};

	return OBB
	{
		.center = {position.x + rotatedOffset.x * scale.x, position.y + rotatedOffset.y * scale.y},
		.halfExtents = {halfExtents.x * scale.x, halfExtents.y * scale.y},
		.rotation = rotation.z
	};
}

float BoxCollider2D::ComputeInertia(const float mass) const
{
	const OBB obb = GetWorldOBB();
	const float w = obb.halfExtents.x * 2.f;
	const float h = obb.halfExtents.y * 2.f;

	return mass * (w * w + h * h) / 12.0f;
}
