#pragma once

#include "pch.h"

#include <atomic>
#include <directxtk12/SimpleMath.h>
#include <Jolt/Physics/Character/CharacterVirtual.h> 
#include <directxtk12/Mouse.h>
#include "Core/Camera.h"
#include "Core/Physics/PhysicsEngine.h"
#include "StepTimer.h"

namespace Gradient
{
    class PlayerCharacter
    {
    public:
        PlayerCharacter();

        void Update(const DX::StepTimer& timer);
        void PhysicsUpdate(const float& deltaTime,
            JPH::Ref<JPH::CharacterVirtual> character,
            JPH::PhysicsSystem* system);
        void SetPosition(DirectX::SimpleMath::Vector3 const&);
        void SetAspectRatio(const float& aspectRatio);
        const Camera& GetCamera() const;

        void Activate();
        void Deactivate();
        bool IsActive();

    private:
        std::tuple<DirectX::SimpleMath::Vector3, bool> GetMovementInput(
            DirectX::SimpleMath::Vector3 right,
            DirectX::SimpleMath::Vector3 forward);

        std::tuple<DirectX::SimpleMath::Vector3,
            DirectX::SimpleMath::Vector3,
            DirectX::SimpleMath::Vector3> GetBasisVectorsThreadSafe();

        void UpdateCamera(DirectX::SimpleMath::Vector3 characterPosition);

        std::atomic_flag m_isActive;
        std::shared_mutex m_cameraMutex;
        Camera m_camera;
        DirectX::Mouse::ButtonStateTracker m_mouseButtonTracker;
        Physics::PhysicsEngine::CharacterID m_character;
    };
}