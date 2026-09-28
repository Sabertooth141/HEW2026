#pragma once
#include "Bindable.h"

class Sampler : public Bindable
{
public:
	Sampler(Renderer& renderer, UINT slot = 0);
	void Bind(Renderer& renderer) override;

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> pSampler;
	UINT slot;
};