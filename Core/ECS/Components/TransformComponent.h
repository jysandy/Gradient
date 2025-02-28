#pragma once

#include "pch.h"

#include <directxtk12/SimpleMath.h>

namespace Gradient::ECS::Components
{
    struct TransformComponent
    {
        DirectX::SimpleMath::Matrix Scale
            = DirectX::SimpleMath::Matrix::Identity;
        DirectX::SimpleMath::Matrix Rotation
            = DirectX::SimpleMath::Matrix::Identity;
        DirectX::SimpleMath::Matrix Translation
            = DirectX::SimpleMath::Matrix::Identity;

        DirectX::SimpleMath::Matrix GetWorldMatrix() const;
        DirectX::SimpleMath::Vector3 GetRotationYawPitchRoll() const;
        DirectX::SimpleMath::Vector3 GetTranslation() const;
    };
}