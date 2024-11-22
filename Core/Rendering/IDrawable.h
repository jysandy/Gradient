#pragma once

#include "pch.h"

#include "Core/Pipelines/IRenderPipeline.h"

namespace Gradient::Rendering
{
    class IDrawable
    {
    public:
        virtual ~IDrawable() = default;

        virtual void Draw(ID3D11DeviceContext* context) = 0;
    };
}