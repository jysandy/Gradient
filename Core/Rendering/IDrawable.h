#pragma once

#include "pch.h"

namespace Gradient::Rendering
{
    class IDrawable
    {
    public:
        virtual ~IDrawable() = default;

        virtual void Draw(ID3D12GraphicsCommandList* cl, int numInstances=1) = 0;
    };
}