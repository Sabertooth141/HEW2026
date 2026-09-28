#include "VertexShader.h"

#pragma comment(lib, "d3dcompiler.lib")

VertexShader::VertexShader(Renderer& renderer, const std::wstring path)
{
	D3DReadFileToBlob(path.c_str(), &pByteCodeBlob);
	GetDevice(renderer)->CreateVertexShader(pByteCodeBlob->GetBufferPointer(),
	                                        pByteCodeBlob->GetBufferSize(),
	                                        nullptr,
	                                        &pVertexShader);
}

void VertexShader::Bind(Renderer& renderer)
{
	GetContext(renderer)->VSSetShader(pVertexShader.Get(), nullptr, 0);
}

ID3DBlob* VertexShader::GetByteCode() const
{
	return pByteCodeBlob.Get();
}
