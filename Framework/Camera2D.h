#pragma once
#include <DirectXMath.h>

class Camera2D
{
public:
    DirectX::XMFLOAT2 GetPosition() const { return position; }
    void SetPosition(const DirectX::XMFLOAT2& p) { position = p; }

    DirectX::XMMATRIX GetView() const
    {
        return DirectX::XMMatrixLookAtLH(
            { position.x, position.y, -5.f },
            { position.x, position.y,  0.f },
            { 0.f, 1.f, 0.f });
    }

private:
    DirectX::XMFLOAT2 position = { 0.f, 0.f };
};