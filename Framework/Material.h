#pragma once
#include <DirectXMath.h>

struct MaterialData
{
	DirectX::XMFLOAT3 color;
	float specularIntensity;
	float specularPower;

	DirectX::XMFLOAT3 padding;
};
