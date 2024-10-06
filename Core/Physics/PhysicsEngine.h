#pragma once
#include "pch.h"

#include <thread>

#include "Core/Physics/Layers.h"
#include "StepTimer.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace Gradient::Physics
{
    class PhysicsEngine
    {
    public:
        const static JPH::uint cMaxBodies = 65536;
        const static JPH::uint cNumBodyMutexes = 0; // Default settings
        const static JPH::uint cMaxBodyPairs = 65536;
        const static JPH::uint cMaxContactConstraints = 10240;

        ~PhysicsEngine();

        static PhysicsEngine* Get();
        static void Initialize();
        static void Shutdown();

        void StartSimulation();
        void StopSimulation();
        void SetTimeScale(float timeScale);

        JPH::BodyInterface& GetBodyInterface();

    private:
        PhysicsEngine();
        static std::unique_ptr<PhysicsEngine> s_engine;

        bool m_isShutDown;
        bool m_workerShouldStop;
        float m_timeScale = 1.f;

        std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator;

        // TODO: Replace this with a custom job system used across the engine
        std::unique_ptr<JPH::JobSystemThreadPool> m_jobSystem;
        std::unique_ptr<JPH::PhysicsSystem> m_physicsSystem;
        std::unique_ptr<std::thread> m_simulationWorker;
        DX::StepTimer m_stepTimer;

#pragma region Callback classes

        // Class that determines if two object layers can collide
        class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
        {
        public:
            virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
        };

        // This defines a mapping between object and broadphase layers.
        class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
        {
        public:
            BPLayerInterfaceImpl();

            virtual JPH::uint GetNumBroadPhaseLayers() const override;
            virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

        private:
            JPH::BroadPhaseLayer m_objectToBroadPhase[ObjectLayers::NUM_LAYERS];
        };

        // Class that determines if an object layer can collide with a broadphase layer
        class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
        {
        public:
            virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
        };

#pragma endregion

        BPLayerInterfaceImpl m_bpLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl m_objectVsBPLayerFilter;
        ObjectLayerPairFilterImpl m_objectLayerPairFilter;
    };
}