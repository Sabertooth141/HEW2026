#pragma once
#include <DirectXMath.h>

struct LightData
{
	DirectX::XMFLOAT3 lightPos;
	float padding0;

	DirectX::XMFLOAT3 ambient;
	float padding1;

	DirectX::XMFLOAT3 diffuseColor;
	float diffuseIntensity;

	float attConst;
	float attLin;
	float attQuad;
	float padding2;
};
