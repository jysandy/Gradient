#pragma once

#include "pch.h"

#include "Core/ECS/EntityManager.h"
#include "Core/TextureManager.h"
#include "Core/ECS/Components/NameTagComponent.h"
#include "Core/ECS/Components/DrawableComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/MaterialComponent.h"
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/ECS/Components/PointLightComponent.h"
#include "Core/ECS/Components/InstanceDataComponent.h"
#include "Core/ECS/Components/RelationshipComponent.h"


namespace Gradient::Scene
{
    entt::entity AddEntity(const std::string& name)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();

        auto e = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(e, name);
        entityManager->Registry.emplace<TransformComponent>(e);

        return e;
    }

    entt::entity AddSphere(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        const float& diameter,
        const Gradient::Rendering::PBRMaterial& material,
        std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn = nullptr)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto sphere1 = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(sphere1, name);
        entityManager->Registry.emplace<TransformComponent>(sphere1);
        entityManager->Registry.emplace<DrawableComponent>(sphere1,
            Rendering::ProceduralMesh::CreateSphere(device,
                cq, diameter)
        );
        entityManager->Registry.emplace<MaterialComponent>(sphere1,
            material);
        entityManager->Registry.emplace<RigidBodyComponent>(sphere1,
            RigidBodyComponent::CreateSphere(diameter,
                position,
                settingsFn
            ));

        return sphere1;
    }

    entt::entity AddBox(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        const DirectX::SimpleMath::Vector3& dimensions,
        const Gradient::Rendering::PBRMaterial& material,
        std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn = nullptr)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto floor = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(floor, name);
        auto& floorTransform =
            entityManager->Registry.emplace<TransformComponent>(floor);
        floorTransform.Translation = Matrix::CreateTranslation(position);
        entityManager->Registry.emplace<DrawableComponent>(floor,
            Rendering::ProceduralMesh::CreateBox(device,
                cq, dimensions));
        entityManager->Registry.emplace<MaterialComponent>(floor,
            material);
        entityManager->Registry.emplace<RigidBodyComponent>(floor,
            RigidBodyComponent::CreateBox(
                dimensions,
                position,
                settingsFn
            ));

        return floor;
    }

    entt::entity AddTerrain(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        int width,
        float height,
        const std::string& textureKey,
        const std::wstring& assetPath,
        std::function<JPH::BodyCreationSettings(JPH::BodyCreationSettings)> settingsFn = nullptr)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto terrain = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(terrain, name);
        auto& terrainTransform = entityManager->Registry.emplace<TransformComponent>(terrain);
        terrainTransform.Translation = Matrix::CreateTranslation(position);
        entityManager->Registry.emplace<DrawableComponent>(terrain,
            Rendering::ProceduralMesh::CreateGrid(device,
                cq,
                width,
                width,
                25,
                false),
            DrawableComponent::ShadingModel::Heightmap,
            false);
        entityManager->Registry.emplace<HeightMapComponent>(terrain,
            textureManager->GetTexture(textureKey),
            height,
            static_cast<float>(width));
        entityManager->Registry.emplace<RigidBodyComponent>(terrain,
            RigidBodyComponent::CreateHeightField(assetPath,
                static_cast<float>(width),
                height,
                position,
                settingsFn));

        return terrain;
    }

    entt::entity AddTree(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        Rendering::LSystem& lsystem,
        const std::string& startingRule,
        int numGenerations,
        const float leafWidth = 0.5f)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();


        // Tree

        auto tree = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(tree, name + "Trunk");
        auto& frustumTransform
            = entityManager->Registry.emplace<TransformComponent>(tree);
        frustumTransform.Translation = Matrix::CreateTranslation(position);

        entityManager->Registry.emplace<DrawableComponent>(tree,
            lsystem.GetMesh(device, cq, startingRule, numGenerations));

        entityManager->Registry.emplace<MaterialComponent>(tree,
            Rendering::PBRMaterial(
                "bark_albedo",
                "bark_normal",
                "bark_ao",
                "defaultMetalness",
                "bark_roughness"
            ));

        // End tree

        // Leaves

        auto leaves = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(leaves, name + "Leaves");
        auto& leavesTransform
            = entityManager->Registry.emplace<TransformComponent>(leaves);
        entityManager->Registry.emplace<RelationshipComponent>(leaves, tree);
        entityManager->Registry.emplace<MaterialComponent>(leaves,
            Rendering::PBRMaterial(
                "leaf_albedo",
                "leaf_normal",
                "leaf_ao",
                "defaultMetalness",
                "leaf_roughness"
            ));

        // Instance generation

        int numRows = 3;
        int numCols = 4;

        auto& leavesInstance
            = entityManager->Registry.emplace<InstanceDataComponent>(leaves);

        Matrix billboardTransform = Matrix::Identity;

        // Shift the leaf origin and point it upwards.
        billboardTransform *= Matrix::CreateTranslation({ 0, 0, -leafWidth / 2.f })
            * Matrix::CreateRotationX(DirectX::XM_PIDIV2);

        for (const auto& transform : lsystem.GetLeafTransforms())
        {
            float colIndex = abs(rand()) % numCols;
            float rowIndex = abs(rand()) % numRows;

            leavesInstance.Instances.push_back({
                    billboardTransform 
                        * Matrix::CreateFromQuaternion(transform.Rotation)
                        * Matrix::CreateTranslation(transform.Translation),
                    Vector2{colIndex / numCols, (colIndex + 1.f) / numCols},
                    Vector2{rowIndex / numRows, (rowIndex + 1.f) / numRows}
                });
        }

        entityManager->Registry.emplace<DrawableComponent>(leaves,
            Rendering::ProceduralMesh::CreateBillboard(device,
                cq,
                leafWidth,
                leafWidth));

        // End leaves

        return tree;
    }


    void CreateScene(ID3D12Device* device, ID3D12CommandQueue* cq)
    {
        using namespace Gradient::ECS::Components;

        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();

        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();


        AddSphere(device, cq,
            "sphere1",
            Vector3{ -3.f, 3.f + 10.f, 0.f },
            1.f,
            Rendering::PBRMaterial(
                "metalSAlbedo",
                "metalSNormal",
                "metalSAO",
                "metalSMetalness",
                "metalSRoughness"
            ),
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                settings.mLinearVelocity = JPH::Vec3{ 0, 1.5f, 0 };
                return settings;
            });

        AddSphere(device, cq,
            "sphere2",
            Vector3{ 3.f, 5.f + 10.f, 0.f },
            1.f,
            Rendering::PBRMaterial(
                "ornamentAlbedo",
                "ornamentNormal",
                "ornamentAO",
                "ornamentMetalness",
                "ornamentRoughness"
            ),
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                return settings;
            });

        Rendering::LSystem lsystem;
        lsystem.AddRule('T', "FFF[/+FX[--G]][////+FX[++G]]/////////+FX[-G]");
        lsystem.AddRule('X', "F[/+FX[--G]][////+FX[++G]]/////////+FX[-G]");
        lsystem.AddRule('G', "[//--L][//---L][\\^++L][\\\\&&++L]");
        lsystem.AddRule('L', "L///+^L");
        lsystem.StartingRadius = 0.3f;
        lsystem.RadiusFactor = 0.5f;
        lsystem.AngleDegrees = 25.7f;
        lsystem.MoveDistance = 1.f;

        lsystem.Build("T", 3);

        Rendering::LSystem twigSystem;
        twigSystem.AddRule('X', "FF/-F+F[--B]//F[^^B]//-FB");
        twigSystem.AddRule('B', "FG[//^^B]F[\\\\&L]GG[+B]-B");
        twigSystem.AddRule('G', "F[//--L][//---L][\\^++L][\\\\&&++L][\\\\&&+++L]");
        twigSystem.AddRule('L', "L///+^L");
        twigSystem.StartingRadius = 0.02f;
        twigSystem.RadiusFactor = 1.f;
        twigSystem.AngleDegrees = 20.f;
        twigSystem.MoveDistance = 0.2f;

        twigSystem.Build("X", 5);

        lsystem.Combine(twigSystem);

        AddTree(device, cq, "tree1",
            { 38.6f, 8.7f, 0.f },
            lsystem, "X", 5, 0.20f);

        Rendering::LSystem bushSystem;
        bushSystem.AddRule('T', "FFFX");
        bushSystem.AddRule('X', "F[/+FB[--L]][////+FB[++L]]/////////+FB[-L]");
        bushSystem.AddRule('B', "FF[/+FB[-L-L]][////+F[+L+L]B[+L+L]]/////////+F[+L+L]B");
        bushSystem.AddRule('L', "+^L/&--L/&&+L");
        bushSystem.StartingRadius = 0.01f;
        bushSystem.RadiusFactor = 1.f;
        bushSystem.AngleDegrees = 25.7f;
        bushSystem.MoveDistance = 0.05f;

        AddTree(device, cq, "bush1",
            { 33.6f, 8.7f, 0.f },
            bushSystem, "T", 6, 0.06f);


        AddBox(device, cq,
            "floor",
            Vector3{ 0.f, -0.25f + 10.f, 0.f },
            Vector3{ 20.f, 0.5f, 20.f },
            Rendering::PBRMaterial(
                "tiles06Albedo",
                "tiles06Normal",
                "tiles06AO",
                "tiles06Metalness",
                "tiles06Roughness"
            ),
            [](JPH::BodyCreationSettings settings)
            {
                settings.mMotionType = JPH::EMotionType::Static;
                settings.mObjectLayer = Gradient::Physics::ObjectLayers::NON_MOVING;
                return settings;
            });

        AddBox(device, cq,
            "box1",
            Vector3{ -5.f, 1.5f + 10.f, -4.f },
            Vector3{ 1, 1, 1 },
            Rendering::PBRMaterial(
                "metal01Albedo",
                "metal01Normal",
                "metal01AO",
                "metal01Metalness",
                "metal01Roughness"
            ));

        AddBox(device, cq,
            "box2",
            Vector3{ -5.f, 1.5f + 10.f, 1.f },
            Vector3{ 1, 1, 1 },
            Rendering::PBRMaterial(
                "crate",
                "crateNormal",
                "crateAO",
                "defaultMetalness",
                "crateRoughness"
            ));

        auto water = AddEntity("water");
        entityManager->Registry.emplace<DrawableComponent>(water,
            Rendering::ProceduralMesh::CreateGrid(device,
                cq,
                800,
                800,
                100),
            DrawableComponent::ShadingModel::Water);

        auto ePointLight1 = AddSphere(device, cq,
            "pointLight1",
            Vector3{ -5.f, 20.f, 5.f },
            0.5f,
            Rendering::PBRMaterial::Light(7.f,
                { 0.9, 0.8, 0.5 }),
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                return settings;
            });

        auto& pointLightComponent
            = entityManager->Registry.emplace<PointLightComponent>(ePointLight1);
        pointLightComponent.PointLight.Colour = Color(Vector3{ 0.9, 0.8, 0.5 });
        pointLightComponent.PointLight.Irradiance = 7.f;
        pointLightComponent.PointLight.MaxRange = 10.f;
        pointLightComponent.PointLight.ShadowCubeIndex = 0;

        auto ePointLight2 = AddSphere(device, cq,
            "pointLight2",
            { 8.f, 20, 0.f },
            0.5f,
            Rendering::PBRMaterial::Light(7.f,
                { 1, 0.3, 0 }),
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                return settings;
            });

        auto& pointLight2Component
            = entityManager->Registry.emplace<PointLightComponent>(ePointLight2);
        pointLight2Component.PointLight.Colour = Color(Vector3{ 1, 0.3, 0 });
        pointLight2Component.PointLight.Irradiance = 7.f;
        pointLight2Component.PointLight.MaxRange = 10.f;
        pointLight2Component.PointLight.ShadowCubeIndex = 1;


        textureManager->LoadDDS(device, cq,
            "islandHeightMap",
            L"Assets\\island_height_32bit.dds");

        AddTerrain(device, cq,
            "terrain",
            { 50, -1, 0 },
            256,
            10,
            "islandHeightMap",
            L"Assets\\island_height_32bit.dds");
    }
}