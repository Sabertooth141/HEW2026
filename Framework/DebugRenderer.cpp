#include "DebugRenderer.h"

#include "InputLayout.h"
#include "PixelShader.h"
#include "VertexShader.h"

namespace
{
	struct DebugTransforms
	{
		DirectX::XMMATRIX model;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};
}

DebugRenderer::DebugRenderer(Renderer& renderer)
{
	vertexShader = std::make_unique<VertexShader>(renderer, L"DebugVertexShader.cso");
	pixelShader = std::make_unique<PixelShader>(renderer, L"DebugPixelShader.cso");

	std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	inputLayout = std::make_unique<InputLayout>(renderer, layout, vertexShader->GetByteCode());

	DebugTransforms init{};
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(DebugTransforms);
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &init;
	renderer.pDevice->CreateBuffer(&cbd, &csd, &transformCBuffer);

	vertices.reserve(256);
}

DebugRenderer::~DebugRenderer() = default;

void DebugRenderer::Begin()
{
	vertices.clear();
}

void DebugRenderer::DrawLine(DirectX::XMFLOAT2 start, DirectX::XMFLOAT2 end, DirectX::XMFLOAT4 color)
{
	vertices.push_back({{start.x, start.y, debugDepth}, color});
	vertices.push_back({{end.x, end.y, debugDepth}, color});
}

void DebugRenderer::DrawBox(const std::array<DirectX::XMFLOAT2, 4>& corners, DirectX::XMFLOAT4 color)
{
	DrawLine(corners[0], corners[1], color);
	DrawLine(corners[1], corners[2], color);
	DrawLine(corners[2], corners[3], color);
	DrawLine(corners[3], corners[0], color);
}

void DebugRenderer::DrawCross(DirectX::XMFLOAT2 center, const float size, DirectX::XMFLOAT4 color)
{
	DrawLine({center.x - size, center.y}, {center.x + size, center.y}, color);
	DrawLine({center.x, center.y - size}, {center.x, center.y + size}, color);
}

void DebugRenderer::Flush(Renderer& renderer)
{
	if (vertices.empty())
	{
		return;
	}

	auto* ctx = renderer.pContext.Get();

	EnsureCapacity(renderer, vertices.size());
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, vertices.data(), sizeof(DebugVertex) * vertices.size());
		ctx->Unmap(vertexBuffer.Get(), 0);
	}

	{
		DebugTransforms t;
		t.model = DirectX::XMMatrixIdentity();
		t.view = DirectX::XMMatrixTranspose(renderer.GetView());
		t.projection = DirectX::XMMatrixTranspose(renderer.GetProj()); // active proj

		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(transformCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, &t, sizeof(t));
		ctx->Unmap(transformCBuffer.Get(), 0);
	}

	// pipeline
	vertexShader->Bind(renderer);
	pixelShader->Bind(renderer);
	inputLayout->Bind(renderer);
	ctx->VSSetConstantBuffers(0, 1, transformCBuffer.GetAddressOf());
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	const UINT stride = sizeof(DebugVertex);
	const UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

	// draw at the end 
	ctx->Draw(static_cast<UINT>(vertices.size()), 0);
}

void DebugRenderer::EnsureCapacity(const Renderer& renderer, const size_t vertexCount)
{
	if (vertexCount <= vertexBufferCapacity && vertexBuffer)
	{
		return;
	}

	vertexBufferCapacity = vertexCount * 2 + 64;

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = static_cast<UINT>(sizeof(DebugVertex) * vertexBufferCapacity);
	bd.StructureByteStride = sizeof(DebugVertex);

	vertexBuffer.Reset();
	renderer.pDevice->CreateBuffer(&bd, nullptr, &vertexBuffer);
}
