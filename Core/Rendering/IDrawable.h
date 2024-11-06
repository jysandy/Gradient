#pragma once

#include "pch.h"

#include "Core/Effects/IEntityEffect.h"

namespace Gradient::Rendering
{
    class IDrawable
    {
    public:
        virtual ~IDrawable() = default;

        virtual void Draw(Effects::IEntityEffect* effect, std::function<void()> setCustomState) = 0;
    };
}