#pragma once

#include "pch.h"

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
        float pad;
    };
}