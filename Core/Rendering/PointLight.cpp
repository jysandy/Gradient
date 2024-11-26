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
                MaxRange
            };
        }

        return Params::PointLight{
            Colour,
            Irradiance,
            entity->GetTranslation(),
            MaxRange
        };
    }
}