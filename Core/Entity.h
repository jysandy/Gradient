#pragma once

#include <directxtk/SimpleMath.h>
#include <directxtk/GeometricPrimitive.h>
#include <memory>
#include <string>

namespace Gradient
{
    struct Entity
    {
        std::string id;

        DirectX::SimpleMath::Matrix Scale;
        DirectX::SimpleMath::Matrix Rotation;
        DirectX::SimpleMath::Matrix Translation;
        std::unique_ptr<DirectX::GeometricPrimitive> Primitive;

        Entity();
        DirectX::SimpleMath::Matrix GetWorldMatrix() const;
        void OnDeviceLost();
    };
}
