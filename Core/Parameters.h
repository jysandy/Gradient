#pragma once

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
}

