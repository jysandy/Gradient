#pragma once

#include "pch.h"
#include <directxtk/SimpleMath.h>

#include "Core/Parameters.h"

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
        std::string EntityId;
        DirectX::SimpleMath::Color Colour = { 1.f, 1.f, 1.f, 1.f };
        float Irradiance = 3.f;

        Params::PointLight AsParams() const;

    private:                   

    };
}