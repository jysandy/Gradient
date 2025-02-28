#pragma once

#include "pch.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <set>
#include <directxtk12/SimpleMath.h>
#include <entt/entt.hpp>
#include "StepTimer.h"


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
}