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
#include "Core/Math.h"
#include "Core/Logger.h"

#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Core/RTTI.h>

#include "Core/Physics/Conversions.h"


namespace Gradient::Scene
{

    struct InstanceEntityData
    {
        BufferManager::MeshHandle MeshHandle;
        BufferManager::InstanceBufferHandle InstanceBufferHandle;
        std::vector<BufferManager::InstanceData> Instances;
    };

    struct Tree
    {
        BufferManager::MeshHandle Trunk;
        InstanceEntityData Branches;
        InstanceEntityData Leaves;
        float TrunkRadius;
        Rendering::PBRMaterial BarkMaterial;
        Rendering::PBRMaterial LeafMaterial;
        DirectX::XMFLOAT2 LeafDimensions;
    };

    entt::entity AddEntity(const std::string& name)
    {
        using namespace Gradient::ECS::Components;
        auto entityManager = EntityManager::Get();

        auto e = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(e, name);
        entityManager->Registry.emplace<TransformComponent>(e);

        return e;
    }

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

    void AttachBillboards(entt::entity entity,
        BufferManager::InstanceBufferHandle instanceBufferHandle,
        const std::vector<BufferManager::InstanceData>& instances,
        DirectX::XMFLOAT2 dimensions)
    {
        using namespace Gradient::ECS::Components;
        auto em = EntityManager::Get();
        auto bm = BufferManager::Get();

        auto& leavesInstance
            = em->Registry.emplace<InstanceDataComponent>(
                entity,
                instanceBufferHandle);

        DirectX::BoundingBox bb;
        DirectX::BoundingBox::CreateFromPoints(bb,
            DirectX::SimpleMath::Vector3(-dimensions.x / 2.f, 0, -dimensions.y / 2.f),
            DirectX::SimpleMath::Vector3(dimensions.x / 2.f, 0, dimensions.y / 2.f));

        em->Registry.emplace<BoundingBoxComponent>(
            entity,
            BoundingBoxComponent::CreateFromInstanceData(
                bb,
                instances
            ));

        em->Registry.emplace<DrawableComponent>(entity,
            BufferManager::MeshHandle(),
            DrawableComponent::ShadingModel::Billboard,
            true,
            dimensions);
    }

    void AttachInstances(entt::entity entity,
        const InstanceEntityData& data)
    {
        AttachInstances(entity, data.MeshHandle, data.InstanceBufferHandle, data.Instances);
    }

    void AttachBillboards(entt::entity entity,
        const InstanceEntityData& data,
        DirectX::XMFLOAT2 dimensions)
    {
        AttachBillboards(entity, data.InstanceBufferHandle, data.Instances, dimensions);
    }

    void AttachBillboards(entt::entity entity,
        const InstanceEntityData& data,
        float width)
    {
        AttachBillboards(entity, data.InstanceBufferHandle, data.Instances, DirectX::XMFLOAT2(width, width));
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
                "forest_floor_albedo",
                "forest_floor_normal",
                "forest_floor_ao",
                "forest_floor_roughness",
                100.f
            ));

        return terrain;
    }

    InstanceEntityData MakeLeaves(ID3D12Device* device, ID3D12CommandQueue* cq,
        Rendering::LSystem& lsystem, DirectX::XMFLOAT2 leafDimensions = { 0.5f, 0.5f })
    {
        auto bm = BufferManager::Get();

        InstanceEntityData out;

        Matrix billboardTransform = Matrix::Identity;

        // Shift the leaf origin and point it upwards.
        billboardTransform *= Matrix::CreateTranslation({ 0, 0, -leafDimensions.y / 2.f })
            * Matrix::CreateRotationX(DirectX::XM_PIDIV2);

        int numRows = 3;
        int numCols = 4;

        for (const auto& transform : lsystem.GetLeafTransforms())
        {
            float colIndex = abs(rand()) % numCols;
            float rowIndex = abs(rand()) % numRows;

            Matrix netTransform = billboardTransform
                * Matrix::CreateFromQuaternion(transform.Rotation)
                * Matrix::CreateTranslation(transform.Translation);

            Vector3 netScale;
            Quaternion netRotation;
            Vector3 netTranslation;
            netTransform.Decompose(netScale, netRotation, netTranslation);

            out.Instances.push_back({
                    netTranslation,
                    1.f,
                    netRotation,
                    Vector2{colIndex / numCols, (colIndex + 1.f) / numCols},
                    Vector2{rowIndex / numRows, (rowIndex + 1.f) / numRows}
                });
        }

        out.InstanceBufferHandle = bm->CreateInstanceBuffer(device,
            cq,
            out.Instances);

        return out;
    }

    InstanceEntityData MakeLeaves(ID3D12Device* device, ID3D12CommandQueue* cq,
        Rendering::LSystem& trunk, Rendering::LSystem& branches,
        uint32_t numRows, uint32_t numCols, uint32_t maxRow, uint32_t maxCol,
        DirectX::XMFLOAT2 leafDimensions = { 0.5f, 0.5f })
    {
        auto bm = BufferManager::Get();

        InstanceEntityData out;

        Matrix billboardTransform = Matrix::Identity;

        // Shift the leaf origin and point it upwards.
        billboardTransform *= Matrix::CreateTranslation({ 0, 0, -leafDimensions.y / 2.f })
            * Matrix::CreateRotationX(DirectX::XM_PIDIV2);

        auto leafTransforms = trunk.GetCombinedLeaves(branches);

        for (const auto& transform : leafTransforms)
        {
            float colIndex = abs(rand()) % (maxCol + 1);
            float rowIndex = abs(rand()) % (maxRow + 1);

            Matrix netTransform = billboardTransform
                * Matrix::CreateFromQuaternion(transform.Rotation)
                * Matrix::CreateTranslation(transform.Translation);

            Vector3 netScale;
            Quaternion netRotation;
            Vector3 netTranslation;
            netTransform.Decompose(netScale, netRotation, netTranslation);

            out.Instances.push_back({
                    netTranslation,
                    1.f,
                    netRotation,
                    Vector2{colIndex / numCols, (colIndex + 1.f) / numCols},
                    Vector2{rowIndex / numRows, (rowIndex + 1.f) / numRows}
                });
        }

        // TODO: Sort instances by sub-UVs to maximise warp coherence
        // TODO: Make the leaf texture smaller

        out.InstanceBufferHandle = bm->CreateInstanceBuffer(device,
            cq,
            out.Instances);

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
                    transform.Translation,
                    1.f,
                    transform.Rotation,
                    Vector2{0, 1},
                    Vector2{0, 1}
                });
        }

        out.InstanceBufferHandle = bm->CreateInstanceBuffer(device,
            cq,
            out.Instances);

        out.MeshHandle = bm->CreateFromPart(device, cq, branches.GetTrunk(), 0.4f, 0.1f);

        return out;
    }

    entt::entity AddBush(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        BufferManager::MeshHandle trunkMeshHandle,
        const InstanceEntityData& leafData,
        const float leafWidth = 0.5f)
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
        frustumTransform.Rotation = Matrix::CreateFromAxisAngle(Vector3::UnitY, rand() / DirectX::XM_2PI);

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
                "leaf_roughness",
                1.f,
                true
            ));

        AttachBillboards(leaves, leafData, leafWidth);

        return tree;
    }

    entt::entity AddTree(ID3D12Device* device, ID3D12CommandQueue* cq,
        const std::string& name,
        const DirectX::SimpleMath::Vector3& position,
        BufferManager::MeshHandle trunkMeshHandle,
        const InstanceEntityData& branchData,
        const InstanceEntityData& leafData,
        Rendering::PBRMaterial barkMaterial,
        Rendering::PBRMaterial leafMaterial,
        const DirectX::XMFLOAT2 leafDimensions = { 0.5f, 0.5f },
        const float trunkRadius = 0.3f)
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
        frustumTransform.Rotation = Matrix::CreateFromAxisAngle(Vector3::UnitY, rand() / DirectX::XM_2PI);

        AttachMeshWithBB(tree, trunkMeshHandle);

        entityManager->Registry.emplace<MaterialComponent>(tree,
            barkMaterial);

        auto cylinderHeight = 3.f;
        entityManager->Registry.emplace<RigidBodyComponent>(tree,
            RigidBodyComponent::CreateCylinder(
                cylinderHeight,
                2 * trunkRadius,
                { position.x, position.y + cylinderHeight / 2.f, position.z },
                [](JPH::BodyCreationSettings settings)
                {
                    settings.mMotionType = JPH::EMotionType::Static;
                    settings.mObjectLayer = Gradient::Physics::ObjectLayers::NON_MOVING;
                    return settings;
                }
            ));

        // Branches
        auto branches = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(branches, name + "Branches");
        auto& branchesTransform
            = entityManager->Registry.emplace<TransformComponent>(branches);
        entityManager->Registry.emplace<RelationshipComponent>(branches, tree);
        entityManager->Registry.emplace<MaterialComponent>(branches,
            barkMaterial);

        AttachInstances(branches, branchData);

        // Leaves
        auto leaves = entityManager->AddEntity();
        entityManager->Registry.emplace<NameTagComponent>(leaves, name + "Leaves");
        auto& leavesTransform
            = entityManager->Registry.emplace<TransformComponent>(leaves);
        entityManager->Registry.emplace<RelationshipComponent>(leaves, tree);
        entityManager->Registry.emplace<MaterialComponent>(leaves,
            leafMaterial);

        AttachBillboards(leaves, leafData, leafDimensions);

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

    // Places a point onto a height field.
    // point is in the xz plane, in the local space of the height field.
    Vector3 PlaceOntoHeightField(const JPH::HeightFieldShape* hfShape,
        Matrix hfWorld,
        Vector2 point,
        float offset = 0.f)
    {
        Matrix hfWorldInverse = hfWorld.Invert();

        JPH::RVec3 out;
        JPH::RVec3 in = Physics::ToJolt(Vector3::Transform({ point.x, 0, point.y }, hfWorldInverse));
        JPH::SubShapeID ignored;

        hfShape->ProjectOntoSurface(in, out, ignored);
        return Vector3::Transform(Physics::FromJolt(out), hfWorld)
            - Vector3{ 0, offset, 0 };
    }

    void CreateScene(ID3D12Device* device, ID3D12CommandQueue* cq)
    {
        using namespace Gradient::ECS::Components;

        auto entityManager = EntityManager::Get();
        auto textureManager = TextureManager::Get();
        auto bm = BufferManager::Get();

        JPH::BodyInterface& bodyInterface
            = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

        //CreatePointLights(device, cq, true);
        //CreateDemoObjects(device, cq);

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

        auto terrain = AddTerrain(device, cq,
            "terrain",
            { 0, -1, 0 },
            256,
            10,
            "islandHeightMap",
            L"Assets\\island_height_32bit.dds");

        std::vector<Tree> treeTypes;

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
        treeBranch.Build("X", 5, 3);

        auto trunkMesh = bm->CreateFromPart(device, cq, treeTrunk.GetTrunk(), 0.1f, 0.1f);
        auto branchData = MakeBranches(device, cq, treeTrunk, treeBranch);
        auto leafData = MakeLeaves(device, cq, treeTrunk, treeBranch, 3, 4, 2, 3, { 0.17f, 0.20f });
        Logger::Get()->info("Tree 1 has " + std::to_string(leafData.Instances.size()) + " leaves");


        treeTypes.push_back({
            trunkMesh,
            branchData,
            leafData,
            0.3f,
            Rendering::PBRMaterial(
                "bark_albedo",
                "bark_normal",
                "bark_ao",
                "bark_roughness",
                2.f
            ),
            Rendering::PBRMaterial(
                "leaf_albedo",
                "leaf_normal",
                "leaf_ao",
                "defaultMetalness",
                "leaf_roughness",
                1.f,
                true
            ),
            { 0.20f, 0.20f }
            });

        Rendering::LSystem treeTrunk2;
        treeTrunk2.AddRule('T', "F^F&F[/+FX[-G]][///+F^X[++G]]//////w/+F&X[-G][//^-L]");
        treeTrunk2.AddRule('X', "F[^/+FX[-G][//^-L]][///+&FX[++G][//^-L]]///////+F^X[-G][//^-L]");
        treeTrunk2.AddRule('G', "[//-L][/+L]");
        //treeTrunk2.AddRule('L', "L///+^L");
        treeTrunk2.StartingRadius = 0.2f;
        treeTrunk2.RadiusFactor = 0.5f;
        treeTrunk2.AngleDegrees = 30.f;
        treeTrunk2.MoveDistance = 1.5f;
        treeTrunk2.Build("T", 3);

        Rendering::LSystem treeBranch2;
        treeBranch2.AddRule('X', "F^F/-F+F[--B]//F[^^B]//-FB");
        treeBranch2.AddRule('B', "F^[//^^B]F&[\\\\&L]G[+B]-BG");
        treeBranch2.AddRule('G', "F[//--L][//---L][\\^++L]");
        treeBranch2.StartingRadius = 0.04f;
        treeBranch2.RadiusFactor = 0.95f;
        treeBranch2.AngleDegrees = 30.f;
        treeBranch2.MoveDistance = 0.2f;
        treeBranch2.Build("X", 5, 3);

        auto trunkMesh2 = bm->CreateFromPart(device, cq, treeTrunk2.GetTrunk(), 0.1f, 0.1f);
        auto branchData2 = MakeBranches(device, cq, treeTrunk2, treeBranch2);
        auto leafData2 = MakeLeaves(device, cq, treeTrunk2, treeBranch2, 3, 7, 0, 0, { 0.10f, 0.20f });
        Logger::Get()->info("Tree 2 has " + std::to_string(leafData2.Instances.size()) + " leaves");


        treeTypes.push_back({
            trunkMesh2,
            branchData2,
            leafData2,
            0.2f,
            Rendering::PBRMaterial(
                "bark2_albedo",
                "bark2_normal",
                "bark2_ao",
                "bark2_roughness",
                2.f
            ),
            Rendering::PBRMaterial(
                "bay_leaf_albedo",
                "bay_leaf_normal",
                "bay_leaf_ao",
                "bay_leaf_roughness",
                1.f,
                true
            ),
            {0.10f, 0.20f}
            });

        Rendering::LSystem treeTrunk3;
        treeTrunk3.AddRule('T', "FFF&F[/+FX[-G]][///+FX[--B]]///////+F^X[-B][//^-L]");
        treeTrunk3.AddRule('X', "F[^//+BX][///-BX]////--BX[//^-L]");
        treeTrunk3.AddRule('B', "FG");
        treeTrunk3.AddRule('G', "[//&&&-L][/^^^+L]");
        treeTrunk3.StartingRadius = 0.25f;
        treeTrunk3.RadiusFactor = 0.7f;
        treeTrunk3.AngleDegrees = 20.f;
        treeTrunk3.MoveDistance = 0.8f;
        treeTrunk3.Build("T", 4);

        Rendering::LSystem treeBranch3;
        treeBranch3.AddRule('X', "F^^F//-F+F[--B]///F[^^B]//-FB");
        treeBranch3.AddRule('B', "F^[//^^B]F&G[+B]-BG");
        treeBranch3.AddRule('G', "F[///--L][//---L][\\\\&&+++L]");
        treeBranch3.StartingRadius = 0.03f;
        treeBranch3.RadiusFactor = 0.95f;
        treeBranch3.AngleDegrees = 40.f;
        treeBranch3.MoveDistance = 0.2f;
        treeBranch3.Build("X", 5, 3);

        auto trunkMesh3 = bm->CreateFromPart(device, cq, treeTrunk3.GetTrunk(), 0.1f, 0.1f);
        auto branchData3 = MakeBranches(device, cq, treeTrunk3, treeBranch3);
        auto leafData3 = MakeLeaves(device, cq, treeTrunk3, treeBranch3, 3, 7, 0, 0, { 0.10f, 0.20f });
        Logger::Get()->info("Tree 3 has " + std::to_string(leafData3.Instances.size()) + " leaves");


        treeTypes.push_back({
            trunkMesh3,
            branchData3,
            leafData3,
            0.25f,
            Rendering::PBRMaterial(
                "bark3_albedo",
                "bark3_normal",
                "bark3_ao",
                "bark3_roughness",
                2.f
            ),
            Rendering::PBRMaterial(
                "bay_leaf_albedo",
                "bay_leaf_normal",
                "bay_leaf_ao",
                "bay_leaf_roughness",
                1.f,
                true
            ),
            {0.10f, 0.20f}
            });

        std::vector<Vector2> treePositions
            = Math::GeneratePoissonDiskSamples(150, 75, 3.f);

        Logger::Get()->info("Generated " + std::to_string(treePositions.size()) + " trees");

        auto& terrainBody = entityManager->Registry.get<RigidBodyComponent>(terrain);
        auto hfWorld = entityManager->GetWorldMatrix(terrain);

        auto shape = bodyInterface.GetShape(terrainBody.BodyID);
        const JPH::HeightFieldShape* hfShape = JPH::StaticCast<JPH::HeightFieldShape>(shape);

        size_t leafCount = 0;

        for (int i = 0; i < treePositions.size(); i++)
        {
            auto treeIndex = rand() % treeTypes.size();

            AddTree(device, cq, "tree" + std::to_string(i),
                PlaceOntoHeightField(hfShape,
                    hfWorld,
                    treePositions[i],
                    0.02),
                treeTypes[treeIndex].Trunk,
                treeTypes[treeIndex].Branches,
                treeTypes[treeIndex].Leaves,
                treeTypes[treeIndex].BarkMaterial,
                treeTypes[treeIndex].LeafMaterial,
                treeTypes[treeIndex].LeafDimensions,
                treeTypes[treeIndex].TrunkRadius);

            leafCount += treeTypes[treeIndex].Leaves.Instances.size();
        }

        struct Bush
        {
            BufferManager::MeshHandle Trunk;
            InstanceEntityData Leaves;
        };

        std::vector<Bush> bushTypes;

        Rendering::LSystem bushSystem;
        bushSystem.AddRule('T', "FFFX");
        bushSystem.AddRule('X', "F[/+FB[--L]][////+FB[++L]]/////////+FB[-L]");
        bushSystem.AddRule('B', "FF[/+FB[-L-L]][////+F[+L+L]B[+L+L]]/////////+F[+L+L]B");
        //bushSystem.AddRule('L', "+^L/&--L/&&+L");
        //bushSystem.AddRule('L', "+^L/&--L");
        bushSystem.StartingRadius = 0.01f;
        bushSystem.RadiusFactor = 1.f;
        bushSystem.AngleDegrees = 25.7f;
        bushSystem.MoveDistance = 0.05f;

        bushSystem.Build("T", 6, 3);

        bushTypes.push_back({
               bm->CreateFromPart(device, cq, bushSystem.GetTrunk(), 0.4f, 0.2f),
               MakeLeaves(device, cq, bushSystem, {0.06f, 0.06f})
            });

        Rendering::LSystem bushSystem2;
        bushSystem2.AddRule('T', "FF\\FX");
        bushSystem2.AddRule('X', "F[/+FB[--L]][//&//+FB[++L]]//&/+FB[-L]");
        bushSystem2.AddRule('B', "FF[/+FB[-L-L]][//&//+F[+L+L]B[+L+L]]/^//^/+F[+L+L]B");
        bushSystem2.StartingRadius = 0.01f;
        bushSystem2.RadiusFactor = 1.f;
        bushSystem2.AngleDegrees = 20.f;
        bushSystem2.MoveDistance = 0.07f;

        bushSystem2.Build("T", 5, 3);

        bushTypes.push_back({
                bm->CreateFromPart(device, cq, bushSystem2.GetTrunk(), 0.4f, 0.2f),
                MakeLeaves(device, cq, bushSystem2, {0.06f, 0.06f})
            });

        Rendering::LSystem bushSystem3;
        bushSystem3.AddRule('T', "FX");
        bushSystem3.AddRule('X', "F[/+FB[--L]][////+FB[++L]]/////////+FB[-L]");
        bushSystem3.AddRule('B', "FF[/+FB[-L]][////+F[+L+L]B[+L]]/////////+F[+L]B");
        bushSystem3.StartingRadius = 0.015f;
        bushSystem3.RadiusFactor = 1.f;
        bushSystem3.AngleDegrees = 30.f;
        bushSystem3.MoveDistance = 0.05f;

        bushSystem3.Build("T", 8, 3);

        bushTypes.push_back({
               bm->CreateFromPart(device, cq, bushSystem3.GetTrunk(), 0.4f, 0.2f),
               MakeLeaves(device, cq, bushSystem3, {0.03f, 0.05f})
            });

        for (int i = 0; i < bushTypes.size(); i++)
        {
            Logger::Get()->info("Bush " 
                + std::to_string(i + 1) 
                + " has "
                + std::to_string(bushTypes[i].Leaves.Instances.size()) 
                + " leaves");
        }

        // To position bushes, rotate the tree position by 90 degrees 
        // and scale it down slightly. Then jitter a bit
        auto bushPositionTransform = Matrix::CreateRotationY(DirectX::XMConvertToRadians(90))
            * Matrix::CreateScale(0.8);

        size_t bushCount = 0;
        for (int i = 0; i < treePositions.size(); i++)
        {
            auto clusterPosition = Vector3(treePositions[i].x, 0, treePositions[i].y);
            clusterPosition = Vector3::Transform(clusterPosition, bushPositionTransform);

            int numBushes = rand() % 10;
            for (int j = 0; j < numBushes; j++)
            {
                auto bushPosition = clusterPosition
                    + Vector3((rand() % 150 + 33) / 33.f, 0, (rand() % 150 + 33) / 33.f);

                auto bushIndex = rand() % bushTypes.size();

                AddBush(device, cq, "bush" + std::to_string(i) + "-" + std::to_string(j),
                    PlaceOntoHeightField(hfShape,
                        hfWorld,
                        Vector2(bushPosition.x, bushPosition.z),
                        0.02),
                    bushTypes[bushIndex].Trunk, bushTypes[bushIndex].Leaves, 0.06f);
                leafCount += bushTypes[bushIndex].Leaves.Instances.size();
                bushCount++;
            }
        }

        Logger::Get()->info("Generated " + std::to_string(bushCount) + " bushes");
        Logger::Get()->info("Generated a total of " + std::to_string(leafCount) + " leaves");
    }
}