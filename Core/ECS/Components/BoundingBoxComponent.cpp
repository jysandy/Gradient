#include "pch.h"

#include "Core/ECS/Components/BoundingBoxComponent.h"

namespace Gradient::ECS::Components
{
    void BoundingBoxComponent::Merge(const DirectX::BoundingBox& other,
        const DirectX::SimpleMath::Matrix& transform)
    {
        auto in = BoundingBox;
        DirectX::BoundingBox transformedOther;
        other.Transform(transformedOther, transform);
        DirectX::BoundingBox::CreateMerged(BoundingBox,
            in,
            transformedOther);
    }

    BoundingBoxComponent BoundingBoxComponent::CreateFromInstanceData(
        const DirectX::BoundingBox& instanceBox,
        const std::vector<BufferManager::InstanceData>& instances
    )
    {
        auto mergedBox = BoundingBoxComponent{ instanceBox };

        for (const auto& instance : instances)
        {
            mergedBox.Merge(instanceBox, instance.World);
        }

        return mergedBox;
    }
}