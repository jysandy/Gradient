#pragma once

#include "pch.h"

#include "Core/Rendering/IDrawable.h"

namespace Gradient::ECS::Components
{
    struct DrawableComponent
    {
        enum class ShadingModel
        {
            Default,
            Water,
            Heightmap
        };

        std::unique_ptr<Rendering::IDrawable> Drawable;
        ShadingModel ShadingModel = ShadingModel::Default;
        bool CastsShadows = true;
    };
}