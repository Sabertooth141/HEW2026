#pragma once
#include "IComponent.h"

struct TextLabel : public IComponent
{
	std::string text;
	DirectX::XMFLOAT2 posOffset = {0.f, 0.f};
	bool isDrawingScreen = false;
};