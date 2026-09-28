#pragma once
#include "Win.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>

class Bindable;

struct SpriteData
{
	int flipX;
	int flipY;
	float padding[2];
};

class Renderer
{
	friend class Bindable;
	friend class DebugRenderer;

public:
	Renderer(HWND hWnd, int width, int height);
	Renderer(const Renderer&) = default;
	Renderer& operator =(const Renderer&) = default;
	~Renderer() = default;

	void BeginFrame(float r, float g, float b);
	void EndFrame();
	void DrawIndexed(UINT count);

	DirectX::XMMATRIX GetView()
	{
		return viewMatrix;
	}

	DirectX::XMMATRIX GetProj()
	{
		return activeProj;
	}

	Microsoft::WRL::ComPtr<ID3D11Device> GetDevice()
	{
		return pDevice;
	}

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext()
	{
		return pContext;
	}

	void SetDepthEnabled(bool enabled);
	void SetAlphaEnabled(bool enabled);
	void SetSpriteFlip(bool flipX, bool flipY);
	void SetView(DirectX::XMMATRIX inView);

	void Set3DMode();
	void Set2DMode();

private:
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV;

	// depth states
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSStateOn; // 3D DSS ON
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSStateOff; // 2D DSS off

	// blend states
	Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendOff; // 3D
	Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendAlpha; // 2D

	// buffer for flipX/Y
	Microsoft::WRL::ComPtr<ID3D11Buffer> pCBSpriteData;

	DirectX::XMMATRIX viewMatrix;

	// use ortho for 2D and perspective for 3D
	DirectX::XMMATRIX projMatrix;
	DirectX::XMMATRIX orthoMatrix;

	DirectX::XMMATRIX activeProj = {};
};
