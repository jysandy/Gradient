#pragma once

#include "pch.h"

#include <directxtk12/SimpleMath.h>
#include "Core/BufferManager.h"

namespace Gradient::ECS::Components
{
    struct BoundingBoxComponent
    {
        DirectX::BoundingBox BoundingBox;

        void Merge(const DirectX::BoundingBox& other,
            const DirectX::SimpleMath::Matrix& transform = DirectX::SimpleMath::Matrix::Identity);

        static BoundingBoxComponent CreateFromInstanceData(
            const DirectX::BoundingBox& instanceBox,
            const std::vector<BufferManager::InstanceData>& instances
        );
    };
}