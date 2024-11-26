#pragma once

#include <directxtk/SimpleMath.h>

namespace Gradient::Params
{
    struct WaterScattering
    {
        float ThicknessPower = 3.f;
        float Sharpness = 4.f;
        float RefractiveIndex = 1.5f;
    };

    struct Water
    {
        float MinLod = 50.f;
        float MaxLod = 400.f;
        WaterScattering Scattering;
    };

    struct PointLight
    {
        DirectX::SimpleMath::Color Colour = { 1.f, 1.f, 1.f, 1.f };
        float Irradiance = 3.f;
        DirectX::SimpleMath::Vector3 Position = { 0.f, 0.f, 0.f };
        float MaxRange = 30.f;
    };
}

