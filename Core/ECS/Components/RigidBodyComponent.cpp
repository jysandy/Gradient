#include "pch.h"

#include "Core/Physics/PhysicsEngine.h"
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/Physics/Conversions.h"

#include <DirectXTex.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <wincodec.h>

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
        size_t sampleOffset = 1;
        assert((info.width - 1) % sampleOffset == 0);
        size_t sampleCount = ((info.width - 1) / sampleOffset) + 1;
        float scaleFactor = gridWidth / ((float)sampleCount - 1.f);
        JPH::RVec3 offset = { -gridWidth / 2.f, 0, -gridWidth / 2.f };
        JPH::RVec3 scale = { scaleFactor, height ,scaleFactor };
        DirectX::EvaluateImage(*image->GetImage(0, 0, 0),
            [&](const DirectX::XMVECTOR* pixels, size_t width, size_t y)
            {
                if (y % sampleOffset != 0) return;

                for (size_t j = 0; j < width; j += sampleOffset)
                {
                    Vector4 color = pixels[j];
                    heightData.push_back(color.x);
                }
            });


        // Create the body.
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto shapeSettings = JPH::HeightFieldShapeSettings(heightData.data(),
            offset,
            scale,
            sampleCount);
        shapeSettings.mBlockSize = 2;
        auto bitsPerSample = shapeSettings.CalculateBitsPerSampleForError(0.001);

        shapeSettings.mBitsPerSample = 8;

        JPH::Shape::ShapeResult result;
        auto heightFieldShape = new JPH::HeightFieldShape(shapeSettings, result);

        if (!result.IsValid())
        {
            throw std::runtime_error(result.GetError().c_str());
        }

        //float* joltHeights = new float[2048 * 2048];
        //heightFieldShape->GetHeights(0, 0, sampleCount, sampleCount,
        //    joltHeights,
        //    2048);
        //DirectX::ScratchImage newHeightmap;
        //DX::ThrowIfFailed(
        //    newHeightmap.Initialize(info));

        //auto hr = DirectX::TransformImage(*image->GetImage(0, 0, 0),
        //    [=](DirectX::XMVECTOR* outPixels,
        //        const DirectX::XMVECTOR* inPixels,
        //        size_t width, size_t y)
        //    {
        //        for (size_t j = 0; j < width; ++j)
        //        {
        //            Vector4 out = { 0, 0, 0, 0 };
        //            out.x = joltHeights[y * sampleCount + j] / height;

        //            outPixels[j] = out;
        //        }
        //    }, newHeightmap);

        //delete[] joltHeights;
        //DX::ThrowIfFailed(hr);

        //DX::ThrowIfFailed(
        //    DirectX::SaveToWICFile(*newHeightmap.GetImage(0, 0, 0),
        //        DirectX::WIC_FLAGS_NONE,
        //        DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
        //        L"island_height_jolt.png"
        //    //    &GUID_WICPixelFormat32bppGrayFloat
        //    ));
        //DX::ThrowIfFailed(
        //  DirectX::SaveToDDSFile(*newHeightmap.GetImage(0, 0, 0),
        //      DirectX::DDS_FLAGS_NONE,
        //      L"island_height_jolt.dds"));

        //float mse = 0.f;
        //DirectX::ComputeMSE(*image->GetImage(0, 0, 0),
        //    *newHeightmap.GetImage(0, 0, 0),
        //    mse,
        //    nullptr,
        //    DirectX::CMSE_IGNORE_GREEN | DirectX::CMSE_IGNORE_BLUE | DirectX::CMSE_IGNORE_ALPHA);

        JPH::BodyCreationSettings settings(
            heightFieldShape,
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