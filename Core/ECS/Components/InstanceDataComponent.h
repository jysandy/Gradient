#pragma once

#include "pch.h"

#include "Core/BufferManager.h"
#include <directxtk12/SimpleMath.h>

namespace Gradient::ECS::Components
{
    struct InstanceDataComponent
    {
        BufferManager::InstanceBufferHandle BufferHandle;
    };
}