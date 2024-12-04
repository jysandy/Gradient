#include "pch.h"

#include "Core/Rendering/PointLight.h"
#include "Core/EntityManager.h"

namespace Gradient::Rendering
{
    Params::PointLight PointLight::AsParams() const
    {
        auto entityManager = EntityManager::Get();
        auto entity = entityManager->LookupEntity(EntityId);

        if (entity == nullptr)
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

        return Params::PointLight{
            Colour,
            Irradiance,
            entity->GetTranslation(),
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