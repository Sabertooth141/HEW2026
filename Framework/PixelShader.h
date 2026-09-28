#pragma once
#include <string>
#include <d3dcompiler.h>

#include "Bindable.h"

class PixelShader : public Bindable
{
public:
	PixelShader(Renderer& renderer, const std::wstring& path);
	void Bind(Renderer& renderer) override;

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader;
};
