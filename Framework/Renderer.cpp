#include "Renderer.h"

#pragma comment(lib, "d3d11.lib")

Renderer::Renderer(HWND hWnd, int width, int height)
{
	// device & swap chain
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&pSwapChain,
		&pDevice,
		nullptr,
		&pContext
	);

	// render target view
	Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer);
	pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pRTV);

	// depth stencil buffer + view
	// depth ON for 3D
	D3D11_DEPTH_STENCIL_DESC dsOn = {};
	dsOn.DepthEnable = TRUE;
	dsOn.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsOn.DepthFunc = D3D11_COMPARISON_LESS;
	pDevice->CreateDepthStencilState(&dsOn, &pDSStateOn);

	// depth OFF for 2D
	D3D11_DEPTH_STENCIL_DESC dsOff= {};
	dsOff.DepthEnable = FALSE;
	dsOff.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	pDevice->CreateDepthStencilState(&dsOff, &pDSStateOff);

	// default to depth ON
	pContext->OMSetDepthStencilState(pDSStateOn.Get(), 1);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC dtDesc = {};
	dtDesc.Width = width;
	dtDesc.Height = height;
	dtDesc.MipLevels = 1;
	dtDesc.ArraySize = 1;
	dtDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dtDesc.SampleDesc.Count = 1;
	dtDesc.SampleDesc.Quality = 0;
	dtDesc.Usage = D3D11_USAGE_DEFAULT;
	dtDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	pDevice->CreateTexture2D(&dtDesc, nullptr, &pDepthStencil);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	pDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &pDSV);

	// buffer for flipping x and y
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(SpriteData);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pDevice->CreateBuffer(&cbd, nullptr, &pCBSpriteData);

	pContext->PSSetConstantBuffers(3, 1, pCBSpriteData.GetAddressOf());

	// bind render target + depth stencil
	pContext->OMSetRenderTargets(1, pRTV.GetAddressOf(), pDSV.Get());

	// viewport
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	pContext->RSSetViewports(1, &vp);

	// blend states
	// alpha blend ON for 2D
	D3D11_BLEND_DESC bdAlpha = {};
	bdAlpha.RenderTarget[0].BlendEnable = TRUE;
	bdAlpha.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bdAlpha.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bdAlpha.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bdAlpha.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bdAlpha.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bdAlpha.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bdAlpha.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	pDevice->CreateBlendState(&bdAlpha, &pBlendAlpha);

	// alpha blend OFF for 3D
	D3D11_BLEND_DESC bdOff = {};
	bdOff.RenderTarget[0].BlendEnable = FALSE;
	bdOff.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	pDevice->CreateBlendState(&bdOff, &pBlendOff);

	// default to blend OFF
	pContext->OMSetBlendState(pBlendOff.Get(), nullptr, 0xFFFFFFFF);

	// view and proj / ortho matrices
	viewMatrix = DirectX::XMMatrixLookAtLH(

		{0, 0, -5}, // camera position
		{0, 0, 0}, // look target
		{0, 1, 0} // up

	);

	projMatrix = DirectX::XMMatrixPerspectiveFovLH(

		DirectX::XMConvertToRadians(70),
		static_cast<float>(width) / static_cast<float>(height),
		0.1f,
		100.0f
	);

	orthoMatrix = DirectX::XMMatrixOrthographicLH(
		static_cast<float>(width),
		static_cast<float>(height),
		0.1f,
		100.0f
	);

	// After device creation
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D11InfoQueue> pInfoQueue;
	pDevice->QueryInterface(__uuidof(ID3D11InfoQueue), &pInfoQueue);

	D3D11_MESSAGE_ID hide[] =
	{
		D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET
	};

	D3D11_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumIDs = 1;
	filter.DenyList.pIDList = hide;
	pInfoQueue->AddStorageFilterEntries(&filter);
#endif
}

void Renderer::SetDepthEnabled(bool enabled)
{
	pContext->OMSetDepthStencilState(enabled ? pDSStateOn.Get() : pDSStateOff.Get(), 1);
}

void Renderer::SetAlphaEnabled(bool enabled)
{
	pContext->OMSetBlendState(enabled ? pBlendAlpha.Get() : pBlendOff.Get(), nullptr, 0xFFFFFFFF);
}

void Renderer::SetSpriteFlip(bool flipX, bool flipY)
{
	D3D11_MAPPED_SUBRESOURCE msr;
	pContext->Map(pCBSpriteData.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	SpriteData* data = reinterpret_cast<SpriteData*>(msr.pData);
	data->flipX = flipX ? 1 : 0;
	data->flipY = flipY ? 1 : 0;

	pContext->Unmap(pCBSpriteData.Get(), 0);
}

void Renderer::SetView(DirectX::XMMATRIX inView)
{
	viewMatrix = inView;
}

void Renderer::Set3DMode()
{
	SetDepthEnabled(true);
	SetAlphaEnabled(false);
	activeProj = projMatrix;
}

void Renderer::Set2DMode()
{
	SetDepthEnabled(false);
	SetAlphaEnabled(true);
	activeProj = orthoMatrix;
}

void Renderer::BeginFrame(float r, float g, float b)
{
	const float color[] = {r, g, b, 1};
	pContext->ClearRenderTargetView(pRTV.Get(), color);
	pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1, 0);
}

void Renderer::EndFrame()
{
	pSwapChain->Present(1, 0);
}

void Renderer::DrawIndexed(UINT count)
{
	pContext->DrawIndexed(count, 0, 0);
}
