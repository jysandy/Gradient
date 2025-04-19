#pragma once

#include "pch.h"

#include "Core/Rendering/IDrawable.h"
#include "Core/BufferManager.h"

namespace Gradient::ECS::Components
{
    struct DrawableComponent
    {
        enum class ShadingModel
        {
            Default,
            Water,
            Heightmap,
            Billboard
        };

        BufferManager::MeshHandle MeshHandle;
        ShadingModel ShadingModel = ShadingModel::Default;
        bool CastsShadows = true;
        DirectX::XMFLOAT2 BillboardDimensions;
    };
}