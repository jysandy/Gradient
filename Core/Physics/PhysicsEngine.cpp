#include "pch.h"

#include "Core/Logger.h"
#include "Core/Physics/PhysicsEngine.h"

namespace Gradient::Physics
{
    bool PhysicsEngine::ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
    {
        switch (inObject1)
        {
        case ObjectLayers::NON_MOVING:
            return inObject2 == ObjectLayers::MOVING; // Non moving only collides with moving
        case ObjectLayers::MOVING:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }

    PhysicsEngine::BPLayerInterfaceImpl::BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        m_objectToBroadPhase[ObjectLayers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[ObjectLayers::MOVING] = BroadPhaseLayers::MOVING;
    }

    JPH::uint PhysicsEngine::BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer PhysicsEngine::BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
    {
        JPH_ASSERT(inLayer < ObjectLayers::NUM_LAYERS);
        return m_objectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* PhysicsEngine::BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
        default:													JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    bool PhysicsEngine::ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
    {
        switch (inLayer1)
        {
        case ObjectLayers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case ObjectLayers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }

    PhysicsEngine::PhysicsEngine() : m_isShutDown(true), m_workerShouldStop(false)
    {
        m_stepTimer.SetFixedTimeStep(true);
        m_stepTimer.SetTargetElapsedSeconds(1.0 / 60.0);
    }

    PhysicsEngine::~PhysicsEngine()
    {
        Shutdown();
    }

    static void TraceImpl(const char* inFMT, ...)
    {
        // Format the message
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        Logger::Get()->info(buffer);
    }

    void PhysicsEngine::Initialize()
    {
        if (!m_isShutDown) return;

        JPH::RegisterDefaultAllocator();

        // TODO: Install assert callbacks
        JPH::Trace = TraceImpl;

        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            std::max(std::thread::hardware_concurrency() - 4, 2u)
        );

        // TODO: Initialize the physics system and start the update thread
        m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
        m_physicsSystem->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            m_bpLayerInterface,
            m_objectVsBPLayerFilter,
            m_objectLayerPairFilter
        );

        m_isShutDown = false;
    }

    void PhysicsEngine::Shutdown()
    {
        if (!m_isShutDown)
        {
            StopSimulation();
            m_physicsSystem.reset();
            m_jobSystem.reset();
            m_tempAllocator.reset();
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;

            m_isShutDown = true;
        }
    }

    void PhysicsEngine::StartSimulation()
    {
        const float cDeltaTime = 1.0f / 60.0f;
        m_workerShouldStop = false;
        auto simulationWorkerFn = [this, cDeltaTime]() {
            while (!m_workerShouldStop)
            {
                m_stepTimer.Tick([this]()
                    {
                        float deltaTime = m_stepTimer.GetElapsedSeconds();
                        m_physicsSystem->Update(deltaTime,
                            1,
                            m_tempAllocator.get(),
                            m_jobSystem.get());
                    });
            }
            };

        m_simulationWorker = std::make_unique<std::thread>(simulationWorkerFn);
    }

    void PhysicsEngine::StopSimulation()
    {
        if (m_simulationWorker.get() != nullptr)
        {
            m_workerShouldStop = true;
            m_simulationWorker->join();
            m_simulationWorker.reset();
        }
    }

    JPH::BodyInterface& PhysicsEngine::GetBodyInterface()
    {
        return m_physicsSystem->GetBodyInterface();
    }
}