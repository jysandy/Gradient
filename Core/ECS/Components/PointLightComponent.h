#pragma once

#include "pch.h"

#include "Core/Rendering/PointLight.h"

namespace Gradient::ECS::Components
{
    struct PointLightComponent
    {
        Rendering::PointLight PointLight;
    };
}