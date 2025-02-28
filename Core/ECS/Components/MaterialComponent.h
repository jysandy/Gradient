#pragma once

#include "pch.h"

#include "Core/Rendering/PBRMaterial.h"

namespace Gradient::ECS::Components
{
    struct MaterialComponent
    {
        Rendering::PBRMaterial Material;
    };
}