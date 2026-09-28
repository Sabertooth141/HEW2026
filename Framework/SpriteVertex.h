#pragma once
#include <DirectXMath.h>
#include <utility>
#include <vector>


inline MeshData MakeSpriteQuad()
{

    return MeshData{
        {
            { {-0.5f,  0.5f, 0.f}, {0.f, 0.f, 0.f}, {0.f, 0.f} }, // TL
            { { 0.5f,  0.5f, 0.f}, {0.f, 0.f, 0.f}, {1.f, 0.f} }, // TR
            { {-0.5f, -0.5f, 0.f}, {0.f, 0.f, 0.f}, {0.f, 1.f} }, // BL
            { { 0.5f, -0.5f, 0.f}, {0.f, 0.f, 0.f}, {1.f, 1.f} }, // BR
        },
        { 0u,1u,2u, 1u,3u,2u }
    };

}
