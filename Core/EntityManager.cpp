#include "pch.h"

#include "Core/EntityManager.h"
#include <directxtk/GeometricPrimitive.h>
#include <directxtk/SimpleMath.h>
#include <utility>

using namespace DirectX::SimpleMath;

namespace Gradient
{
    EntityManager::EntityManager()
    {
    }

    void EntityManager::UpdateAll(DX::StepTimer const& timer)
    {
        for (auto& entity : m_entities)
        {
            if (entity.Primitive == nullptr) continue;

            if (auto entry = m_updateFunctions.find(entity.id); entry != m_updateFunctions.end())
            {
                entry->second(entity, timer);
            }
        }
    }

    void EntityManager::AddEntity(Entity&& entity)
    {
        m_entities.push_back(std::move(entity));
    }

    void EntityManager::RegisterUpdate(std::string const& entityId, EntityManager::UpdateFunctionType updateFn)
    {
        m_updateFunctions[entityId] = updateFn;
    }

    void EntityManager::AddEntity(Entity&& entity, EntityManager::UpdateFunctionType updateFn)
    {
        this->RegisterUpdate(entity.id, updateFn); // id has to be accessed before moving!
        this->AddEntity(std::move(entity));
    }

    void EntityManager::DeregisterUpdate(std::string const& entityId)
    {
        if (auto it = m_updateFunctions.find(entityId); it != m_updateFunctions.end())
        {
            m_updateFunctions.erase(it);
        }
    }

    void EntityManager::OnDeviceLost()
    {
        for (auto& entity : m_entities)
        {
            entity.OnDeviceLost();
        }
    }

    void EntityManager::DrawAll(Matrix const& view, Matrix const& projection)
    {
        for (auto const& entity : m_entities)
        {
            entity.Primitive->Draw(entity.GetWorldMatrix(),
                view,
                projection);
        }
    }
}
