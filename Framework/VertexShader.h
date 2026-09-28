#pragma once
#include <string>
#include <d3dcompiler.h>

#include "Bindable.h"


class VertexShader : public Bindable
{
public:
	VertexShader(Renderer& renderer, const std::wstring path);
	void Bind(Renderer& renderer) override;
	ID3DBlob* GetByteCode() const;

private:
	Microsoft::WRL::ComPtr<ID3DBlob> pByteCodeBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader = nullptr;
};
