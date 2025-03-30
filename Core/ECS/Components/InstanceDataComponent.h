#pragma once

#include "pch.h"

#include <directxtk12/SimpleMath.h>

namespace Gradient::ECS::Components
{
    struct InstanceDataComponent
    {
        struct Instance
        {
            // This transform is relative to the entity's 
            // TransformComponent.

            DirectX::SimpleMath::Matrix LocalTransform;
            DirectX::SimpleMath::Vector2 TexcoordURange;
            DirectX::SimpleMath::Vector2 TexcoordVRange;
        };

        std::vector<Instance> Instances;
    };
}