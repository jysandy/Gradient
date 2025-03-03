#include "pch.h"

#include "Core/Logger.h"
#include "Core/Physics/PhysicsEngine.h"

namespace Gradient::Physics
{
    std::unique_ptr<PhysicsEngine> PhysicsEngine::s_engine;

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

    PhysicsEngine::PhysicsEngine() : m_isShutDown(true), m_workerShouldStop()
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
        if (s_engine != nullptr) return;

        JPH::RegisterDefaultAllocator();

        // TODO: Install assert callbacks
        JPH::Trace = TraceImpl;

        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        auto engine = new PhysicsEngine();

        s_engine = std::unique_ptr<PhysicsEngine>(engine);

        s_engine->m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        s_engine->m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            std::max(std::thread::hardware_concurrency() - 4, 2u)
        );

        s_engine->m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
        s_engine->m_physicsSystem->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            s_engine->m_bpLayerInterface,
            s_engine->m_objectVsBPLayerFilter,
            s_engine->m_objectLayerPairFilter
        );
    }

    void PhysicsEngine::InitializeDebugRenderer(
        ID3D12Device* device,
        DXGI_FORMAT renderTargetFormat
    )
    {
        if (m_debugRenderer.get() != nullptr) return;
        m_debugRenderer = std::make_unique<DebugRenderer>(device, renderTargetFormat);
    }

    void PhysicsEngine::Shutdown()
    {
        if (s_engine != nullptr)
        {
            s_engine->StopSimulation();
            s_engine->m_physicsSystem.reset();
            s_engine->m_jobSystem.reset();
            s_engine->m_tempAllocator.reset();
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;
            s_engine.reset();
        }
    }

    PhysicsEngine* PhysicsEngine::Get()
    {
        return s_engine.get();
    }

    void PhysicsEngine::StartSimulation()
    {
        m_workerShouldStop.clear();
        m_workerPaused.clear();
        auto simulationWorkerFn = [this]() {
            while (!m_workerShouldStop.test())
            {
                m_stepTimer.Tick([this]()
                    {
                        // Jolt doesn't seem to compute impulses properly if 
                        // the time scale is less than 1. Impulses are weaker 
                        // than they ought to be.
                        // TODO: Fix or remove this feature.
                        float deltaTime = m_stepTimer.GetElapsedSeconds()
                            * m_timeScale;

                        while (deltaTime > 0.f)
                        {
                            m_physicsSystem->Update(
                                std::min(deltaTime, 1.f / 60.f),
                                2,
                                m_tempAllocator.get(),
                                m_jobSystem.get());
                            deltaTime -= 1.f / 60.f;
                        }
                    });

                if (m_workerPaused.test())
                    m_workerPaused.wait(true);
            }
            };

        m_simulationWorker = std::make_unique<std::thread>(simulationWorkerFn);
    }

    void PhysicsEngine::PauseSimulation()
    {
        m_workerPaused.test_and_set();
    }

    void PhysicsEngine::UnpauseSimulation()
    {
        if (IsPaused())
        {
            m_stepTimer.ResetElapsedTime();
            m_workerPaused.clear();
            m_workerPaused.notify_one();
        }
    }

    bool PhysicsEngine::IsPaused()
    {
        return m_workerPaused.test();
    }

    void PhysicsEngine::StopSimulation()
    {
        if (m_simulationWorker.get() != nullptr)
        {
            m_workerShouldStop.test_and_set();
            UnpauseSimulation();
            m_simulationWorker->join();
            m_simulationWorker.reset();
        }
    }

    JPH::BodyInterface& PhysicsEngine::GetBodyInterface()
    {
        return m_physicsSystem->GetBodyInterface();
    }

    void PhysicsEngine::SetTimeScale(float timeScale)
    {
        if (timeScale < 0.1f)
        {
            m_timeScale = 0.1f;
        }
        else if (timeScale > 1.f)
        {
            m_timeScale = 1.f;
        }
        else
        {
            m_timeScale = timeScale;
        }
    }

    void PhysicsEngine::DebugDrawBodies(ID3D12GraphicsCommandList* cl,
        DirectX::SimpleMath::Matrix view,
        DirectX::SimpleMath::Matrix proj,
        DirectX::SimpleMath::Vector3 cameraPos)
    {
        m_debugRenderer->SetFrameVariables(cl,
            view,
            proj,
            cameraPos);

        m_physicsSystem->DrawBodies(JPH::BodyManager::DrawSettings(),
            m_debugRenderer.get());
    }
}