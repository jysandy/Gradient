#pragma once

#include "pch.h"
#include <cstdint>

namespace Gradient::Pipelines
{
    struct AlignedDirectionalLight
    {
        DirectX::XMFLOAT3 colour;
        float irradiance;
        DirectX::XMFLOAT3 direction;
        float pad;
    };

    struct AlignedPointLight
    {
        DirectX::XMFLOAT3 colour;
        float irradiance;
        DirectX::XMFLOAT3 position;
        float maxRange;
        uint32_t shadowCubeIndex;
        float pad[3];
        DirectX::XMMATRIX shadowTransforms[6];
    };

    struct Wave
    {
        float amplitude;
        float wavelength;
        float speed;
        float sharpness;
        DirectX::XMFLOAT3 direction;
        float pad;
    };
}