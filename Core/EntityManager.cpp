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

    void EntityManager::CreateEntities(ID3D11DeviceContext1* deviceContext)
    {
        Entity sphere1;
        sphere1.id = "sphere1";
        sphere1.Primitive = DirectX::GeometricPrimitive::CreateSphere(deviceContext, 2.f);
        sphere1.Translation = Matrix::CreateTranslation(Vector3{ -3.f, 3.f, 0.f });
        this->AddEntity(std::move(sphere1));

        Entity sphere2;
        sphere2.id = "sphere2";
        sphere2.Primitive = DirectX::GeometricPrimitive::CreateSphere(deviceContext, 2.f);
        sphere2.Translation = Matrix::CreateTranslation(Vector3{ 3.f, 3.f, 0.f });
        this->AddEntity(std::move(sphere2));

        Entity floor;
        floor.id = "floor";
        floor.Primitive = DirectX::GeometricPrimitive::CreateBox(deviceContext, Vector3{ 20.f, 0.5f, 20.f });
        floor.Translation = Matrix::CreateTranslation(Vector3{ 0.f, -0.25f, 0.f });
        this->AddEntity(std::move(floor));
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
