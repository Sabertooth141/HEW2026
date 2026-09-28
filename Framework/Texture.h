#pragma once
#include "Bindable.h"

class Texture : public Bindable
{
public:
	Texture(ID3D11ShaderResourceView* inSrv, UINT inSlot = 0) : pSrv(inSrv), slot(inSlot)
	{
	}

	void Bind(Renderer& renderer) override
	{
		GetContext(renderer)->PSSetShaderResources(slot, 1, pSrv.GetAddressOf());
	}

	DirectX::XMFLOAT2 GetTextureSize() const
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> res = nullptr;
		pSrv->GetResource(&res);

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex = nullptr;
		res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

		D3D11_TEXTURE2D_DESC desc;
		tex->GetDesc(&desc);

		return {static_cast<float>(desc.Width), static_cast<float>(desc.Height)};
	}

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pSrv;
	UINT slot;
};
