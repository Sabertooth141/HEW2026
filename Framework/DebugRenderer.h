#pragma once
#include <array>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <wrl/client.h>

#include "ConstantBuffer.h"
class InputLayout;
class PixelShader;
class VertexShader;
class Renderer;

struct DebugVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
};

class DebugRenderer
{
public:
	explicit DebugRenderer(Renderer& renderer);
	~DebugRenderer();

	void Begin();
	void DrawLine(DirectX::XMFLOAT2 start, DirectX::XMFLOAT2 end, DirectX::XMFLOAT4 color);
	void DrawBox(const std::array<DirectX::XMFLOAT2, 4>& corners, DirectX::XMFLOAT4 color);
	void DrawCross(DirectX::XMFLOAT2 center, float size, DirectX::XMFLOAT4 color);
	void Flush(Renderer& renderer);

private:
	void EnsureCapacity(const Renderer& renderer, size_t vertexCount);

private:
	std::vector<DebugVertex> vertices;

	std::unique_ptr<VertexShader> vertexShader;
	std::unique_ptr<PixelShader> pixelShader;
	std::unique_ptr<InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> transformCBuffer;
	size_t vertexBufferCapacity = 0;

	static constexpr float debugDepth = 0.5f;
};
