#include "Sampler.h"

Sampler::Sampler(Renderer& renderer, UINT slot) : slot(slot)
{
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	GetDevice(renderer)->CreateSamplerState(&sd, &pSampler);
}

void Sampler::Bind(Renderer& renderer)
{
	GetContext(renderer)->PSSetSamplers(slot, 1, pSampler.GetAddressOf());
}
