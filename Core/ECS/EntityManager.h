#pragma once

#include "pch.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <set>
#include <optional>
#include <directxtk12/SimpleMath.h>
#include <entt/entt.hpp>
#include "StepTimer.h"

#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/RelationshipComponent.h"

namespace Gradient
{
    class EntityManager
    {
    public:

        static void Initialize();
        static void Shutdown();

        static EntityManager* Get();

        void UpdateAll(DX::StepTimer const&);

        entt::entity AddEntity();

        template <typename T>
        const T* TryGetParentComponent(entt::entity entity) const;

        DirectX::SimpleMath::Matrix GetWorldMatrix(entt::entity entity) const;
        std::optional<DirectX::BoundingBox> GetBoundingBox(entt::entity entity) const;
        
        // Returns a bounding box that can be used to cull 
        // the object when drawing directional shadows.
        std::optional<DirectX::BoundingBox> GetDirectionalShadowBoundingBox(entt::entity entity,
            DirectX::SimpleMath::Vector3 lightDirection) const;

        void SetRotation(entt::entity entity,
            float yaw, float pitch, float roll);
        void SetTranslation(entt::entity entity,
            const DirectX::SimpleMath::Vector3& offset);

        DirectX::SimpleMath::Vector3 GetRotationYawPitchRoll(entt::entity entity) const;
        DirectX::SimpleMath::Vector3 GetTranslation(entt::entity entity) const;

        void OnDeviceLost();

        entt::registry Registry;
    private:
        EntityManager();
        static std::unique_ptr<EntityManager> s_instance;
    };

    template <typename T>
    const T* EntityManager::TryGetParentComponent(entt::entity entity) const
    {
        auto relationship = Registry.try_get<ECS::Components::RelationshipComponent>(entity);
        if (relationship)
        {
            const T* parentComponent
                = Registry.try_get<T>(relationship->Parent);

            return parentComponent;
        }

        return nullptr;
    }
}