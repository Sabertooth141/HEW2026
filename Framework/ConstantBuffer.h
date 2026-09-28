#pragma once
#include "Bindable.h"

template<typename T>
class ConstantBuffer : public Bindable
{
public:
    ConstantBuffer(Renderer& renderer, const T& data)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.ByteWidth = sizeof(T);

        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = &data;

        GetDevice(renderer)->CreateBuffer(&bd, &sd, &pConstantBuffer);
    }

    void Update(Renderer& renderer, const T& data)
    {
        D3D11_MAPPED_SUBRESOURCE msr;
        GetContext(renderer)->Map(
            pConstantBuffer.Get(), 0,
            D3D11_MAP_WRITE_DISCARD, 0, &msr
        );
        memcpy(msr.pData, &data, sizeof(T));
        GetContext(renderer)->Unmap(pConstantBuffer.Get(), 0);
    }

protected:
    Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
};

template<typename T>
class VertexConstantBuffer : public ConstantBuffer<T>
{
    using ConstantBuffer<T>::pConstantBuffer;
    using ConstantBuffer<T>::GetContext;

public:
    VertexConstantBuffer(Renderer& renderer, const T& data, UINT slot = 0) : ConstantBuffer<T>(renderer, data), slot(slot)
	{}
    using ConstantBuffer<T>::ConstantBuffer;

    void Bind(Renderer& renderer) override
    {
        GetContext(renderer)->VSSetConstantBuffers(
            slot, 1, pConstantBuffer.GetAddressOf()
        );
    }

private:
    UINT slot;
};

template<typename T>
class PixelConstantBuffer : public ConstantBuffer<T>
{
    using ConstantBuffer<T>::pConstantBuffer;
    using ConstantBuffer<T>::GetContext;

public:
    PixelConstantBuffer(Renderer& renderer, const T& data, UINT slot = 0) : ConstantBuffer<T>(renderer, data), slot(slot)
	{}
    using ConstantBuffer<T>::ConstantBuffer;

    void Bind(Renderer& renderer) override
    {
        GetContext(renderer)->PSSetConstantBuffers(
            slot, 1, pConstantBuffer.GetAddressOf()
        );
    }

private:
    UINT slot;
};