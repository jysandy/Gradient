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

        // This pointer is not guaranteed to remain valid!
        // TODO: Make the entity contain only handles/IDs, 
        // so that it can be cheaply copied around
        const Entity* LookupEntity(const std::string& id);
        void MutateEntity(const std::string& id, std::function<void(Entity&)>);
        
        void OnDeviceLost();
    
    private:
        // TODO: Make this concurrency safe
        // We can have multiple readers or editors,
        // but adders need to be serialized
        std::vector<Entity> m_entities;
        std::unordered_map<std::string, UpdateFunctionType> m_updateFunctions;
        std::unordered_map<std::string, size_t> m_idToIndex;
    };
}