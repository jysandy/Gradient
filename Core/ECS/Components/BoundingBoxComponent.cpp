#include "pch.h"

#include "Core/ECS/Components/BoundingBoxComponent.h"

#include <directxtk12/SimpleMath.h>

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
        using namespace DirectX::SimpleMath;
        auto mergedBox = BoundingBoxComponent{ instanceBox };

        for (const auto& instance : instances)
        {
            Matrix transform = Matrix::CreateFromQuaternion(instance.RotationQuat)
                * Matrix::CreateTranslation(instance.Position);

            mergedBox.Merge(instanceBox, transform);
        }

        return mergedBox;
    }
}