#pragma once

#include "pch.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <set>
#include <directxtk/SimpleMath.h>
#include "Core/Entity.h"
#include "StepTimer.h"


namespace Gradient
{
    class EntityManager
    {
    public:
        using UpdateFunctionType = std::function<void(Entity&, DX::StepTimer const&)>;

        static void Initialize();
        static void Shutdown();

        static EntityManager* Get();

        void UpdateAll(DX::StepTimer const&);
        void DrawEntity(
            ID3D11DeviceContext* context,
            const Entity& entity, 
            Effects::IEntityEffect* effect,
            bool drawingShadows = false);
        void DrawAll(
            ID3D11DeviceContext* context,
            Effects::IEntityEffect* effect,
            bool drawingShadows = false);

        void AddEntity(Entity&&);
        void AddEntity(Entity&&, UpdateFunctionType);
        void RegisterUpdate(std::string const& entityId, UpdateFunctionType);
        void DeregisterUpdate(std::string const& entityId);
        const std::set<std::string>& GetIDs() const;

        // This pointer is not guaranteed to remain valid!
        // TODO: Make the entity contain only handles/IDs, 
        // so that it can be cheaply copied around
        const Entity* LookupEntity(const std::string& id);
        void MutateEntity(const std::string& id, std::function<void(Entity&)>);

        void OnDeviceLost();

    private:
        EntityManager();
        static std::unique_ptr<EntityManager> s_instance;

        // TODO: Make this concurrency safe
        // We can have multiple readers or editors,
        // but adders need to be serialized
        std::vector<Entity> m_entities;
        std::unordered_map<std::string, UpdateFunctionType> m_updateFunctions;
        std::unordered_map<std::string, size_t> m_idToIndex;

        // This set needs to be ordered since it's used for 
        // UI rendering.
        // Changing m_idToIndex to an ordered map would make 
        // lookups by ID less efficient.
        std::set<std::string> m_idSet;
    };
}