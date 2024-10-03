#pragma once

#include <Jolt/Physics/Body/BodyID.h>

#include <directxtk/SimpleMath.h>
#include <directxtk/GeometricPrimitive.h>
#include <memory>
#include <string>

namespace Gradient
{
    struct Entity
    {
        std::string id;
        JPH::BodyID BodyID;
        // TODO: Add a transform for the physics body

        DirectX::SimpleMath::Matrix Scale;
        DirectX::SimpleMath::Matrix Rotation;
        DirectX::SimpleMath::Matrix Translation;
        std::unique_ptr<DirectX::GeometricPrimitive> Primitive;

        Entity();
        DirectX::SimpleMath::Matrix GetWorldMatrix() const;
        void OnDeviceLost();
    };
}
