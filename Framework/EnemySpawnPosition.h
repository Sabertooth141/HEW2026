#pragma once
#include <cmath>
#include <random>
#include <DirectXMath.h>

// The entire sprite, not just its center, must be outside the camera rectangle.
 inline DirectX::XMFLOAT2 RandomEnemySpawnPosition(
	const DirectX::XMFLOAT2& cameraPosition,
	const DirectX::XMFLOAT2& viewSize,
	const DirectX::XMFLOAT2& spriteSize,
	float margin, float spawnDepth, std::mt19937& random)
{
	const float halfWidth = viewSize.x * 0.5f;
	const float halfHeight = viewSize.y * 0.5f;
	const float padding = std::fmax(1.f, margin)
		+ std::uniform_real_distribution<float>(0.f, std::fmax(0.f, spawnDepth))(random);
	const float outsideX = halfWidth + std::abs(spriteSize.x) * 0.5f + padding;
	const float outsideY = halfHeight + std::abs(spriteSize.y) * 0.5f + padding;
	const float alongEdge = std::uniform_real_distribution<float>(-1.f, 1.f)(random);

	switch (std::uniform_int_distribution<int>(0, 3)(random))
	{
	case 0: return { cameraPosition.x - outsideX, cameraPosition.y + alongEdge * halfHeight };
	case 1: return { cameraPosition.x + outsideX, cameraPosition.y + alongEdge * halfHeight };
	case 2: return { cameraPosition.x + alongEdge * halfWidth, cameraPosition.y - outsideY };
	default: return { cameraPosition.x + alongEdge * halfWidth, cameraPosition.y + outsideY };
	}
}
