#pragma once

#include "pch.h"

#include "Core/Physics/PhysicsEngine.h"
#include <Jolt/Physics/Body/BodyID.h>
#include <directxtk12/SimpleMath.h>

#include <functional>

namespace Gradient::ECS::Components
{
    struct RigidBodyComponent
    {
        // TODO: Add a transform for the physics body
        JPH::BodyID BodyID;

        static RigidBodyComponent CreateSphere(float diameter,
            DirectX::SimpleMath::Vector3 origin = DirectX::SimpleMath::Vector3::Zero,
            std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn = nullptr);

        static RigidBodyComponent CreateBox(DirectX::SimpleMath::Vector3 dimensions,
            DirectX::SimpleMath::Vector3 origin = DirectX::SimpleMath::Vector3::Zero,
            std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn = nullptr);
    };
}