#pragma once

#include "pch.h"
#include <directxtk12/SimpleMath.h>

#include "Core/Parameters.h"
#include "Core/Rendering/DepthCubeArray.h"

namespace Gradient::Rendering
{
    class PointLight
    {
    public:
        PointLight() = default;

        // The point light gets its position 
        // from a linked Entity.
        // The linked Entity can be moved in the 
        // editor, controlled by physics, etc.
        DirectX::SimpleMath::Color Colour = { 1.f, 1.f, 1.f, 1.f };
        float Irradiance = 3.f;
        float MinRange = 0.1f;
        float MaxRange = 30.f;
        uint32_t ShadowCubeIndex = 0.f;

        Params::PointLight AsParams() const;
        void SetParams(Params::PointLight params);
    };
}