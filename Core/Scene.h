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
#include "Core/ECS/Components/BoundingBoxComponent.h"


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

    struct InstanceEntityData
    {
        BufferManager::MeshHandle MeshHandle;
        BufferManager::InstanceBufferHandle InstanceBufferHandle;
        std::vector<BufferManager::InstanceData> Instances;
    };

    void AttachMeshWithBB(entt::entity entity,
        BufferManager::MeshHandle meshHandle)
    {
        using namespace Gradient::ECS::Components;
        auto em = EntityManager::Get();
        auto bm = BufferManager::Get();

        auto mesh = bm->GetMesh(meshHandle);

        em->Registry.emplace<BoundingBoxComponent>(
            entity,
            mesh->GetBoundingBox()
        );

        em->Registry.emplace<DrawableComponent>(
            entity,
            meshHandle
        );
    }

    void AttachInstances(entt::entity entity,
        BufferManager::MeshHandle instancedMeshHandle,
        BufferManager::InstanceBufferHandle instanceBufferHandle,
        const std::vector<BufferManager::InstanceData>& instances)
    {
        using namespace Gradient::ECS::Components;
        auto em = EntityManager::Get();
        auto bm = BufferManager::Get();

        auto instancedMesh = bm->GetMesh(instancedMeshHandle);

        auto& leavesInstance
            = em->Registry.emplace<InstanceDataComponent>(
                entity,
                instanceBufferHandle);

        em->Registry.emplace<BoundingBoxComponent>(
            entity,
            BoundingBoxComponent::CreateFromInstanceData(
                instancedMesh->GetBoundingBox(),
                instances
            ));

        em->Registry.emplace<DrawableComponent>(entity,
            instancedMeshHandle);
    }

    void AttachInstances(entt::entity entity,
        const InstanceEntityData& data)
    {
        AttachInstances(entity, data.MeshHandle, data.InstanceBufferHandle, data.Instances);
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
        auto bm = BufferManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto sphere1 = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(sphere1, name);
        auto& sphereTransform = entityManager->Registry.emplace<TransformComponent>(sphere1);
        sphereTransform.Translation = Matrix::CreateTranslation(position);
        AttachMeshWithBB(sphere1, bm->CreateSphere(device,
            cq, diameter));
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
        auto bm = BufferManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto floor = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(floor, name);
        auto& floorTransform =
            entityManager->Registry.emplace<TransformComponent>(floor);
        floorTransform.Translation = Matrix::CreateTranslation(position);
        AttachMeshWithBB(floor,
            bm->CreateBox(device,
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
        auto bm = BufferManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto terrain = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(terrain, name);
        auto& terrainTransform = entityManager->Registry.emplace<TransformComponent>(terrain);
        terrainTransform.Translation = Matrix::CreateTranslation(position);
        entityManager->Registry.emplace<DrawableComponent>(terrain,
            bm->CreateGrid(device,
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
        // TODO: Make a bounding box
        entityManager->Registry.emplace<RigidBodyComponent>(terrain,
            RigidBodyComponent::CreateHeightField(assetPath,
                static_cast<float>(width),
                height,
                position,
                settingsFn));

        entityManager->Registry.emplace<MaterialComponent>(terrain,
            Rendering::PBRMaterial(
                "tiles06Albedo",
                "tiles06Normal",
                "tiles06AO",
                "tiles06Metalness",
                "tiles06Roughness",
                5.f
            ));

        return terrain;
    }

    InstanceEntityData MakeLeaves(ID3D12Device* device, ID3D12CommandQueue* cq,
        Rendering::LSystem& lsystem, const float leafWidth = 0.5f)
    {
        auto bm = BufferManager::Get();

        InstanceEntityData out;

        Matrix billboardTransform = Matrix::Identity;

        // Shift the leaf origin and point it upwards.
        billboardTransform *= Matrix::CreateTranslation({ 0, 0, -leafWidth / 2.f })
            * Matrix::CreateRotationX(DirectX::XM_PIDIV2);

        int numRows = 3;
        int numCols = 4;

        for (const auto& transform : lsystem.GetLeafTransforms())
        {
            float colIndex = abs(rand()) % numCols;
            float rowIndex = abs(rand()) % numRows;

            out.Instances.push_back({
                    billboardTransform
                        * Matrix::CreateFromQuaternion(transform.Rotation)
                        * Matrix::CreateTranslation(transform.Translation),
                    Vector2{colIndex / numCols, (colIndex + 1.f) / numCols},
                    Vector2{rowIndex / numRows, (rowIndex + 1.f) / numRows}
                });
        }

        out.InstanceBufferHandle = bm->CreateInstanceBuffer(device,
            cq,
            out.Instances);

        out.MeshHandle = bm->CreateBillboard(device,
            cq,
            leafWidth,
            leafWidth);

        return out;
    }

    InstanceEntityData MakeLeaves(ID3D12Device* device, ID3D12CommandQueue* cq,
        Rendering::LSystem& trunk, Rendering::LSystem& branches, const float leafWidth = 0.5f)
    {
        auto bm = BufferManager::Get();

        InstanceEntityData out;

        int numRows = 3;
        int numCols = 4;

        Matrix billboardTransform = Matrix::Identity;

        // Shift the leaf origin and point it upwards.
        billboardTransform *= Matrix::CreateTranslation({ 0, 0, -leafWidth / 2.f })
            * Matrix::CreateRotationX(DirectX::XM_PIDIV2);

        auto leafTransforms = trunk.GetCombinedLeaves(branches);

        for (const auto& transform : leafTransforms)
        {
            float colIndex = abs(rand()) % numCols;
            float rowIndex = abs(rand()) % numRows;

            out.Instances.push_back({
                    billboardTransform
                        * Matrix::CreateFromQuaternion(transform.Rotation)
                        * Matrix::CreateTranslation(transform.Translation),
                    Vector2{colIndex / numCols, (colIndex + 1.f) / numCols},
                    Vector2{rowIndex / numRows, (rowIndex + 1.f) / numRows}
                });
        }

        out.InstanceBufferHandle = bm->CreateInstanceBuffer(device,
            cq,
            out.Instances);

        out.MeshHandle = bm->CreateBillboard(device,
            cq,
            leafWidth,
            leafWidth);

        return out;
    }

    InstanceEntityData MakeBranches(ID3D12Device* device, ID3D12CommandQueue* cq,
        Rendering::LSystem& trunk, Rendering::LSystem& branches)
    {
        auto bm = BufferManager::Get();

        InstanceEntityData out;

        for (const auto& transform : trunk.GetLeafTransforms())
        {
            out.Instances.push_back({
                    Matrix::CreateFromQuaternion(transform.Rotation)
                        * Matrix::CreateTranslation(transform.Translation),
                    Vector2{0, 1},
                    Vector2{0, 1}
                });
        }

        out.InstanceBufferHandle = bm->CreateInstanceBuffer(device,
            cq,
            out.Instances);

        out.MeshHandle = bm->CreateFromPart(device, cq, branches.GetTrunk());

        return out;
    }

    entt::entity AddBush(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        BufferManager::MeshHandle trunkMeshHandle,
        const InstanceEntityData& leafData)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        auto bm = BufferManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();


        // Tree

        auto tree = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(tree, name + "Trunk");
        auto& frustumTransform
            = entityManager->Registry.emplace<TransformComponent>(tree);
        frustumTransform.Translation = Matrix::CreateTranslation(position);

        AttachMeshWithBB(tree, trunkMeshHandle);

        entityManager->Registry.emplace<MaterialComponent>(tree,
            Rendering::PBRMaterial(
                "bark_albedo",
                "bark_normal",
                "bark_ao",
                "defaultMetalness",
                "bark_roughness"
            ));

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

        AttachInstances(leaves, leafData);

        return tree;
    }

    entt::entity AddTree(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        BufferManager::MeshHandle trunkMeshHandle,
        const InstanceEntityData& branchData,
        const InstanceEntityData& leafData)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        auto bm = BufferManager::Get();
        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        // Tree

        auto tree = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(tree, name + "Trunk");
        auto& frustumTransform
            = entityManager->Registry.emplace<TransformComponent>(tree);
        frustumTransform.Translation = Matrix::CreateTranslation(position);

        AttachMeshWithBB(tree, trunkMeshHandle);

        entityManager->Registry.emplace<MaterialComponent>(tree,
            Rendering::PBRMaterial(
                "bark_albedo",
                "bark_normal",
                "bark_ao",
                "defaultMetalness",
                "bark_roughness"
            ));

        // Branches
        auto branches = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(branches, name + "Branches");
        auto& branchesTransform
            = entityManager->Registry.emplace<TransformComponent>(branches);
        entityManager->Registry.emplace<RelationshipComponent>(branches, tree);
        entityManager->Registry.emplace<MaterialComponent>(branches,
            Rendering::PBRMaterial(
                "bark_albedo",
                "bark_normal",
                "bark_ao",
                "defaultMetalness",
                "bark_roughness"
            ));

        AttachInstances(branches, branchData);

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

        AttachInstances(leaves, leafData);

        return tree;
    }


    // Creates some boxes and spheres.
    void CreateDemoObjects(ID3D12Device* device, ID3D12CommandQueue* cq)
    {
        using namespace Gradient::ECS::Components;

        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        auto bm = BufferManager::Get();

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
    }

    // Creates a couple of point lights.
    void CreatePointLights(ID3D12Device* device, ID3D12CommandQueue* cq, bool nonMoving = false)
    {
        using namespace Gradient::ECS::Components;

        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        auto bm = BufferManager::Get();

        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        auto ePointLight1 = AddSphere(device, cq,
            "pointLight1",
            Vector3{ -5.f, 20.f, 5.f },
            0.5f,
            Rendering::PBRMaterial::Light(7.f,
                { 0.9, 0.8, 0.5 }),
            [=](auto settings)
            {
                if (nonMoving)
                {
                    settings.mMotionType = JPH::EMotionType::Static;
                    settings.mObjectLayer = Gradient::Physics::ObjectLayers::NON_MOVING;
                }

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
            [=](auto settings)
            {
                if (nonMoving)
                {
                    settings.mMotionType = JPH::EMotionType::Static;
                    settings.mObjectLayer = Gradient::Physics::ObjectLayers::NON_MOVING;
                }

                settings.mRestitution = 0.9f;
                return settings;
            });

        auto& pointLight2Component
            = entityManager->Registry.emplace<PointLightComponent>(ePointLight2);
        pointLight2Component.PointLight.Colour = Color(Vector3{ 1, 0.3, 0 });
        pointLight2Component.PointLight.Irradiance = 7.f;
        pointLight2Component.PointLight.MaxRange = 10.f;
        pointLight2Component.PointLight.ShadowCubeIndex = 1;
    }

    void CreateScene(ID3D12Device* device, ID3D12CommandQueue* cq)
    {
        using namespace Gradient::ECS::Components;

        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        auto bm = BufferManager::Get();

        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        CreatePointLights(device, cq, true);
        //CreateDemoObjects(device, cq);

        Rendering::LSystem treeTrunk;
        treeTrunk.AddRule('T', "FFF[/+FX[--G]][////+FX[++G]]/////////+FX[-G]");
        treeTrunk.AddRule('X', "F[/+FX[--G]][////+FX[++G]]/////////+FX[-G]");
        treeTrunk.AddRule('G', "[//--L][//---L][\\^++L][\\\\&&++L]");
        treeTrunk.AddRule('L', "L///+^L");
        treeTrunk.StartingRadius = 0.3f;
        treeTrunk.RadiusFactor = 0.5f;
        treeTrunk.AngleDegrees = 25.7f;
        treeTrunk.MoveDistance = 1.f;

        treeTrunk.Build("T", 3);

        Rendering::LSystem treeBranch;
        treeBranch.AddRule('X', "FF/-F+F[--B]//F[^^B]//-FB");
        treeBranch.AddRule('B', "F[//^^B]F[\\\\&L]G[+B]-BG");
        treeBranch.AddRule('G', "F[//--L][//---L][\\^++L][\\\\&&++L][\\\\&&+++L]");
        //treeBranch.AddRule('L', "L///+^L");
        treeBranch.StartingRadius = 0.02f;
        treeBranch.RadiusFactor = 1.f;
        treeBranch.AngleDegrees = 20.f;
        treeBranch.MoveDistance = 0.2f;

        treeBranch.Build("X", 5);

        auto trunkMesh = bm->CreateFromPart(device, cq, treeTrunk.GetTrunk());
        auto branchData = MakeBranches(device, cq, treeTrunk, treeBranch);
        auto leafData = MakeLeaves(device, cq, treeTrunk, treeBranch, 0.20f);

        std::vector<Vector3> treePositions = {
            {75, 3.7, -35.1},
            {41, 6.7, -35.1},
            {-6.4, 3, -71.3},
            {-59.4, 5.4, 7.9},
            {-48.2, 6.1, 26.0},
            {-15.8, 8.8, 0.4},
            {28.7, 7.1, 34.5},
            {-27.2, 4.6, 62.0}
        };

        for (int i = 0; i < treePositions.size(); i++)
        {
            AddTree(device, cq, "tree" + std::to_string(i),
                treePositions[i],
                trunkMesh,
                branchData,
                leafData);
        }

        Rendering::LSystem bushSystem;
        bushSystem.AddRule('T', "FFFX");
        bushSystem.AddRule('X', "F[/+FB[--L]][////+FB[++L]]/////////+FB[-L]");
        bushSystem.AddRule('B', "FF[/+FB[-L-L]][////+F[+L+L]B[+L+L]]/////////+F[+L+L]B");
        bushSystem.AddRule('L', "+^L/&--L/&&+L");
        bushSystem.StartingRadius = 0.01f;
        bushSystem.RadiusFactor = 1.f;
        bushSystem.AngleDegrees = 25.7f;
        bushSystem.MoveDistance = 0.05f;

        bushSystem.Build("T", 6);

        auto bushTrunkMesh = bm->CreateFromPart(device, cq, bushSystem.GetTrunk());
        auto bushLeafData = MakeLeaves(device, cq, bushSystem, 0.06f);

        std::vector<Vector3> bushPositions = {
            {33.6, 7.9, 0},
            {21.7, 0.2, 4.1},
            {2.3, 8.4, 21.2},
            {-0.4, 7.9, 33.4},
            {-11.8, 7.5, 34.2},
            {-26.5, 7.3, 31.1},
            {-36.1, 7.8, 8},
            {-41, 6.9, -22}
        };

        for (int i = 0; i < bushPositions.size(); i++)
        {
            AddBush(device, cq, "bush" + std::to_string(i),
                bushPositions[i],
                bushTrunkMesh, bushLeafData);
        }


        auto water = AddEntity("water");
        entityManager->Registry.emplace<DrawableComponent>(water,
            bm->CreateGrid(device,
                cq,
                800,
                800,
                100),
            DrawableComponent::ShadingModel::Water);

        textureManager->LoadDDS(device, cq,
            "islandHeightMap",
            L"Assets\\island_height_32bit.dds");

        AddTerrain(device, cq,
            "terrain",
            { 0, -1, 0 },
            256,
            10,
            "islandHeightMap",
            L"Assets\\island_height_32bit.dds");
    }
}