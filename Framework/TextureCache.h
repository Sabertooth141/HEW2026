#pragma once
#include <string>
#include <unordered_map>

#include "Bindable.h"

class TextureCache : public Bindable
{
public: 
	static ID3D11ShaderResourceView* Load(Renderer& renderer, const std::wstring& path);
	static void Clear();

	void Bind(Renderer& renderer) override;

	static ID3D11ShaderResourceView* LoadSolid(Renderer& renderer, uint32_t rgba = 0xFFFFFFFF);

private:
	static std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> cache;
};

