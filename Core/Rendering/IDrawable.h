#pragma once

#include "pch.h"

namespace Gradient::Rendering
{
    class IDrawable
    {
    public:
        virtual ~IDrawable() = default;

        virtual void Draw(ID3D12GraphicsCommandList* cl, uint32_t numInstances=1) = 0;
    };
}