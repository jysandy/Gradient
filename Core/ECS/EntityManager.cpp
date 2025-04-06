#include "pch.h"

#include "Core/ECS/EntityManager.h"  
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/DrawableComponent.h"
#include "Core/ECS/Components/BoundingBoxComponent.h"
#include "Core/TextureManager.h"
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/SimpleMath.h>
#include <utility>
#include "Core/Physics/PhysicsEngine.h"

using namespace DirectX::SimpleMath;
using namespace Gradient::ECS::Components;

namespace Gradient
{
    std::unique_ptr<EntityManager> EntityManager::s_instance;

    EntityManager::EntityManager()
    {
    }

    void EntityManager::Initialize()
    {
        auto instance = new EntityManager();
        s_instance = std::unique_ptr<EntityManager>(instance);
    }

    void EntityManager::Shutdown()
    {
        s_instance.reset();
    }

    EntityManager* EntityManager::Get()
    {
        return s_instance.get();
    }

    void EntityManager::UpdateAll(DX::StepTimer const& timer)
    {
        // TODO: Move this to the main Game update loop?

        using namespace Gradient::ECS::Components;

        auto& bodyInterface = Physics::PhysicsEngine::Get()->GetBodyInterface();
        auto view = Registry.view<TransformComponent, RigidBodyComponent>();
        
        for (auto entity : view)
        {
            auto [transform, rigidBody] = view.get(entity);

            if (!rigidBody.BodyID.IsInvalid()
                && bodyInterface.IsActive(rigidBody.BodyID)
                && bodyInterface.GetMotionType(rigidBody.BodyID) != JPH::EMotionType::Static)
            {
                // Assumes the body's center of mass is at 
                // the mesh's model space origin.
                auto joltPosition 
                    = bodyInterface.GetCenterOfMassPosition(rigidBody.BodyID);
                auto joltRotation 
                    = bodyInterface.GetRotation(rigidBody.BodyID);

                transform.Rotation = Matrix::CreateFromQuaternion(
                    Quaternion(joltRotation.GetX(),
                        joltRotation.GetY(),
                        joltRotation.GetZ(),
                        joltRotation.GetW())
                );
                transform.Translation = Matrix::CreateTranslation(
                    joltPosition.GetX(),
                    joltPosition.GetY(),
                    joltPosition.GetZ()
                );
            }
        }
    }

    entt::entity EntityManager::AddEntity()
    {
        auto entity = Registry.create();
        return entity;
    }

    void EntityManager::OnDeviceLost()
    {
        // TODO: Is this necessary?

        auto view = Registry.view<ECS::Components::DrawableComponent>();

        for (auto entity : view)
        {
            auto& drawable = view.get<ECS::Components::DrawableComponent>(entity);
            drawable.Drawable.reset();
        }
    }

    void EntityManager::SetRotation(entt::entity entity,
        float yaw, float pitch, float roll)
    {
        auto& transform 
            = Registry.get<TransformComponent>(entity);

        transform.Rotation = Matrix::CreateFromYawPitchRoll(yaw, pitch, roll);

        auto pRigidBody = Registry.try_get<RigidBodyComponent>(entity);
        if (pRigidBody != nullptr
            && !pRigidBody->BodyID.IsInvalid())
        {
            auto& bodyInterface = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();
            auto rotationQuat = Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
            bodyInterface.SetRotation(pRigidBody->BodyID, JPH::Quat(
                rotationQuat.x,
                rotationQuat.y,
                rotationQuat.z,
                rotationQuat.w),
                JPH::EActivation::Activate
            );
        }
    }

    void EntityManager::SetTranslation(entt::entity entity, const Vector3& offset)
    {
        auto& transform
            = Registry.get<TransformComponent>(entity);

        transform.Translation = Matrix::CreateTranslation(offset);

        auto pRigidBody = Registry.try_get<RigidBodyComponent>(entity);

        if (pRigidBody != nullptr
            && !pRigidBody->BodyID.IsInvalid())
        {
            auto& bodyInterface = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();
            bodyInterface.SetPosition(pRigidBody->BodyID,
                JPH::Vec3(offset.x, offset.y, offset.z),
                JPH::EActivation::Activate
            );
        }
    }

    DirectX::SimpleMath::Vector3 EntityManager::GetRotationYawPitchRoll(
        entt::entity entity) const
    {
        return Registry.get<TransformComponent>(entity).GetRotationYawPitchRoll();
    }

    DirectX::SimpleMath::Vector3 EntityManager::GetTranslation(
        entt::entity entity) const
    {
        return Registry.get<TransformComponent>(entity).GetTranslation();
    }

    DirectX::SimpleMath::Matrix EntityManager::GetWorldMatrix(entt::entity entity) const
    {
        // TODO: Follow the relationship chain all the way up

        DirectX::SimpleMath::Matrix outWorld = DirectX::SimpleMath::Matrix::Identity;

        auto childTransform = Registry.try_get<TransformComponent>(entity);
        if (childTransform)
        {
            outWorld = childTransform->GetWorldMatrix();
        }

        auto parentTransform = TryGetParentComponent<TransformComponent>(entity);

        if (parentTransform)
        {
            outWorld *= parentTransform->GetWorldMatrix();
        }

        return outWorld;
    }

    std::optional<DirectX::BoundingBox> EntityManager::GetBoundingBox(entt::entity entity) const
    {
        auto bbComponent = Registry.try_get<BoundingBoxComponent>(entity);

        if (!bbComponent)
        {
            return std::nullopt;
        }

        auto worldMatrix = GetWorldMatrix(entity);

        DirectX::BoundingBox out;
        bbComponent->BoundingBox.Transform(out, worldMatrix);

        return out;
    }
}
