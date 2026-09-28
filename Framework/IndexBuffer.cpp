#include "IndexBuffer.h"

#pragma comment(lib, "d3d11.lib")
IndexBuffer::IndexBuffer(Renderer& renderer, const std::vector<unsigned int>& indices) : indexCount(
	static_cast<UINT>(indices.size()))
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * indices.size());

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = indices.data();

	GetDevice(renderer)->CreateBuffer(&bd, &sd, &pIndexBuffer);
}

void IndexBuffer::Bind(Renderer& renderer)
{
	GetContext(renderer)->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

UINT IndexBuffer::GetIndexCount() const
{
	return indexCount;
}
