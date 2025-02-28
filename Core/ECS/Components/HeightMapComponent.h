#pragma once

#include "pch.h"

#include "Core/GraphicsMemoryManager.h"

namespace Gradient::ECS::Components
{
    struct HeightMapComponent
    {
        GraphicsMemoryManager::DescriptorView HeightMapTexture;
        float Height;
        float GridWidth;
    };
}