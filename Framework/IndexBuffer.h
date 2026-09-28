#pragma once
#include <vector>

#include "Bindable.h"

class IndexBuffer : public Bindable
{
public:
	IndexBuffer(Renderer& renderer, const std::vector<unsigned int>& indices);
	void Bind(Renderer& renderer) override;
	UINT GetIndexCount() const;
private:
	UINT indexCount;
	Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
};
