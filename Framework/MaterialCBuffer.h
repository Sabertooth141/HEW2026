#pragma once
#include "Bindable.h"
#include "ConstantBuffer.h"
#include "Material.h"

class MaterialCBuffer : public Bindable
{
public:
	MaterialCBuffer(Renderer& renderer, const MaterialData& data);

	void Bind(Renderer& renderer) override;

private:
	PixelConstantBuffer<MaterialData> pBuffer;
};

