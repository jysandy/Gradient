#include "pch.h"

#include "Core/EntityManager.h"
#include "Core/TextureManager.h"
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/SimpleMath.h>
#include <utility>
#include "Core/Physics/PhysicsEngine.h"

using namespace DirectX::SimpleMath;

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
        auto& bodyInterface = Physics::PhysicsEngine::Get()->GetBodyInterface();
        for (auto& entity : m_entities)
        {
            if (!entity.BodyID.IsInvalid()
                && bodyInterface.IsActive(entity.BodyID)
                && bodyInterface.GetMotionType(entity.BodyID) != JPH::EMotionType::Static)
            {
                // Assumes the body's center of mass is at 
                // the mesh's model space origin.
                auto joltPosition = bodyInterface.GetCenterOfMassPosition(entity.BodyID);
                auto joltRotation = bodyInterface.GetRotation(entity.BodyID);

                entity.Rotation = Matrix::CreateFromQuaternion(
                    Quaternion(joltRotation.GetX(),
                        joltRotation.GetY(),
                        joltRotation.GetZ(),
                        joltRotation.GetW())
                );
                entity.Translation = Matrix::CreateTranslation(
                    joltPosition.GetX(),
                    joltPosition.GetY(),
                    joltPosition.GetZ()
                );
            }

            if (auto entry = m_updateFunctions.find(entity.id); entry != m_updateFunctions.end())
            {
                // TODO: Pass the body interface through?
                entry->second(entity, timer);
            }
        }
    }

    void EntityManager::AddEntity(Entity&& entity)
    {
        auto theID = entity.id;
        m_entities.push_back(std::move(entity));
        m_idToIndex.insert({ theID, m_entities.size() - 1 });
        m_idSet.insert(theID);
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

    const std::set<std::string>& EntityManager::GetIDs() const
    {
        return m_idSet;
    }

    void EntityManager::OnDeviceLost()
    {
        for (auto& entity : m_entities)
        {
            entity.OnDeviceLost();
        }
    }

    // TODO: Make the pipeline take an Entity and draw it 
    // instead. 
    // Then the pipeline can take whatever data it needs 
    // from the Entity, rather than having to accept data 
    // passed in from elsewhere.
    void EntityManager::DrawEntity(
        ID3D12GraphicsCommandList* cl,
        const Entity& entity,
        bool drawingShadows)
    {
        if (entity.Drawable.get() == nullptr)
            return;

        auto textureManager = TextureManager::Get();
        auto blankTexture = textureManager->GetTexture("default");
        auto outwardNormalMap = textureManager->GetTexture("defaultNormal");
        auto dielectricMetalnessMap = textureManager->GetTexture("defaultMetalness");
        auto smoothMap = dielectricMetalnessMap;

        if (drawingShadows && !entity.CastsShadows) return;

        Pipelines::IRenderPipeline* pipeline;

        pipeline = entity.RenderPipeline;

        if (!drawingShadows)
        {
            if (entity.Texture)
                pipeline->SetAlbedo(entity.Texture);
            else
                pipeline->SetAlbedo(blankTexture);

            if (entity.NormalMap)
                pipeline->SetNormalMap(entity.NormalMap);
            else
                pipeline->SetNormalMap(outwardNormalMap);

            if (entity.AOMap)
                pipeline->SetAOMap(entity.AOMap);
            else
                pipeline->SetAOMap(blankTexture);

            if (entity.MetalnessMap)
                pipeline->SetMetalnessMap(entity.MetalnessMap);
            else
                pipeline->SetMetalnessMap(dielectricMetalnessMap);

            if (entity.RoughnessMap)
                pipeline->SetRoughnessMap(entity.RoughnessMap);
            else
                pipeline->SetRoughnessMap(blankTexture);

            pipeline->SetEmissiveRadiance(entity.EmissiveRadiance);
        }

        pipeline->SetWorld(entity.GetWorldMatrix());
        if (drawingShadows)
            pipeline->Apply(cl, false, drawingShadows);
        else
            pipeline->Apply(cl, true, drawingShadows);

        entity.Drawable->Draw(cl);
    }

    void EntityManager::DrawAll(
        ID3D12GraphicsCommandList* cl,
        bool drawingShadows)
    {
        //// DEBUGGING
        //DrawEntity(cl, m_entities[0], drawingShadows);
        //return;

        for (auto const& entity : m_entities)
        {
            DrawEntity(cl, entity, drawingShadows);
        }
    }

    const Entity* EntityManager::LookupEntity(const std::string& id)
    {
        if (auto it = m_idToIndex.find(id); it != m_idToIndex.end())
        {
            return &m_entities[it->second];
        }
        return nullptr;
    }

    void EntityManager::MutateEntity(const std::string& id, std::function<void(Entity&)> mutatorFn)
    {
        if (auto it = m_idToIndex.find(id); it != m_idToIndex.end())
        {
            mutatorFn(m_entities[it->second]);
        }
    }
}
