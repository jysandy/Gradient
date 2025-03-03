#include "pch.h"

#include "Core/Physics/PhysicsEngine.h"
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/Physics/Conversions.h"

#include <DirectXTex.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

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

    RigidBodyComponent RigidBodyComponent::CreateHeightField(
        const std::wstring& heightmapPath,
        float gridWidth,
        float height,
        DirectX::SimpleMath::Vector3 origin,
        std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn
    )
    {
        using namespace DirectX::SimpleMath;

        std::vector<float> heightData;
        DirectX::TexMetadata info;

        // Load the heightmap from disk.
        auto image = std::make_unique<DirectX::ScratchImage>();
        DX::ThrowIfFailed(
            DirectX::LoadFromDDSFile(heightmapPath.c_str(),
                DirectX::DDS_FLAGS_NONE,
                &info,
                *image));

        // Read all the height information.
        float scaleFactor = gridWidth / (float)info.width;
        JPH::RVec3 offset = { -gridWidth / 2.f, 0, -gridWidth / 2.f };
        JPH::RVec3 scale = { scaleFactor, 1 ,scaleFactor };
        DirectX::EvaluateImage(*image->GetImage(0, 0, 0),
            [&](const DirectX::XMVECTOR* pixels, size_t width, size_t y)
            {
                UNREFERENCED_PARAMETER(y);

                for (size_t j = 0; j < width; j++)
                {
                    Vector4 color = pixels[j];
                    heightData.push_back(color.x * height);
                }
            });

        // Create the body.
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        JPH::Ref<JPH::Shape> shape;
        auto shapeSettings = JPH::HeightFieldShapeSettings(heightData.data(),
            offset,
            scale,
            info.width);

        JPH::Shape::ShapeResult result = shapeSettings.Create();

        if (!result.IsValid())
        {
            throw std::runtime_error(result.GetError().c_str());
        }

        shape = result.Get();

        JPH::BodyCreationSettings settings(
            shape,
            Physics::ToJolt(origin),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Gradient::Physics::ObjectLayers::NON_MOVING
        );

        if (settingsFn)
            settings = settingsFn(settings);

        // Height fields must be static.
        settings.mMotionType = JPH::EMotionType::Static;
        settings.mObjectLayer = Gradient::Physics::ObjectLayers::NON_MOVING;

        auto bodyId = bodyInterface.CreateAndAddBody(settings,
            JPH::EActivation::Activate);

        return RigidBodyComponent{ bodyId };
    }
}