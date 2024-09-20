#pragma once

#include <directxtk/SimpleMath.h>
#include <directxtk/GeometricPrimitive.h>
#include <memory>

namespace Gradient
{
    struct Entity
    {
        DirectX::SimpleMath::Matrix Scale;
        DirectX::SimpleMath::Matrix Rotation;
        DirectX::SimpleMath::Matrix Translation;
        std::unique_ptr<DirectX::GeometricPrimitive> Primitive;

        Entity();
        DirectX::SimpleMath::Matrix GetWorldMatrix();
        void OnDeviceLost();
    };
}
