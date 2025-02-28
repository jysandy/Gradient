#include "pch.h"

#include "Core/Physics/PhysicsEngine.h"
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/Physics/Conversions.h"

namespace Gradient::ECS::Components
{
    RigidBodyComponent RigidBodyComponent::CreateSphere(float diameter,
        DirectX::SimpleMath::Vector3 origin,
        std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn)
    {
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        JPH::BodyCreationSettings settings(
            new JPH::SphereShape(diameter / 2.f),
            Physics::ToJolt(origin),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            Physics::ObjectLayers::MOVING
        );

        if (settingsFn)
            settings = settingsFn(settings);

        auto bodyId = bodyInterface.CreateAndAddBody(settings,
            JPH::EActivation::Activate);

        return RigidBodyComponent{ bodyId };
    }

    RigidBodyComponent RigidBodyComponent::CreateBox(DirectX::SimpleMath::Vector3 dimensions,
        DirectX::SimpleMath::Vector3 origin,
        std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn)
    {
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        JPH::BoxShape* shape = new JPH::BoxShape(JPH::Vec3(
            dimensions.x / 2.f, dimensions.y / 2.f, dimensions.z / 2.f));

        JPH::BodyCreationSettings settings(
            shape,
            Physics::ToJolt(origin),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            Gradient::Physics::ObjectLayers::MOVING
        );

        if (settingsFn)
            settings = settingsFn(settings);

        auto bodyId = bodyInterface.CreateAndAddBody(settings,
            JPH::EActivation::Activate);

        return RigidBodyComponent{ bodyId };
    }

}