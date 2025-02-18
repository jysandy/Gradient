#include "pch.h"

#include "Core/Rendering/PointLight.h"
#include "Core/ECS/EntityManager.h"

namespace Gradient::Rendering
{
    Params::PointLight PointLight::AsParams() const
    {
        return Params::PointLight{
            Colour,
            Irradiance,
            {0.f, 0.f, 0.f},
            MinRange,
            MaxRange,
            ShadowCubeIndex
        };
    }

    void PointLight::SetParams(Params::PointLight params)
    {
        Colour = params.Colour;
        Irradiance = params.Irradiance;
        MinRange = params.MinRange;
        MaxRange = params.MaxRange;
        ShadowCubeIndex = params.ShadowCubeIndex;
    }
}