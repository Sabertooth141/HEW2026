#pragma once
#include "Bindable.h"
#include "ConstantBuffer.h"
#include "Light.h"

class LightCBuffer : public Bindable
{
public:
	LightCBuffer(Renderer& renderer, const LightData& data);

	void Update(Renderer& renderer, const LightData& newData);
	void Bind(Renderer& renderer) override;

private:
	LightData data;
	PixelConstantBuffer<LightData> pBuffer;
};

