#pragma once

#include "pch.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <directxtk/SimpleMath.h>
#include "Core/Entity.h"
#include "StepTimer.h"


namespace Gradient
{
    class EntityManager
    {
    public:
        using UpdateFunctionType = std::function<void(Entity&, DX::StepTimer const&)>;
    
        EntityManager();

        void UpdateAll(DX::StepTimer const&);
        void DrawAll(DirectX::SimpleMath::Matrix const& view, DirectX::SimpleMath::Matrix const& projection);

        void AddEntity(Entity&&);
        void AddEntity(Entity&&, UpdateFunctionType);
        void RegisterUpdate(std::string const& entityId, UpdateFunctionType);
        void DeregisterUpdate(std::string const& entityId);
        
        void OnDeviceLost();
    
    private:
        std::vector<Entity> m_entities;
        std::unordered_map<std::string, UpdateFunctionType> m_updateFunctions;
    };
}