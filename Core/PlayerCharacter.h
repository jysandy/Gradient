#pragma once

#include "pch.h"

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

    private:
        std::tuple<DirectX::SimpleMath::Vector3, bool> GetMovementInput(
            DirectX::SimpleMath::Vector3 right,
            DirectX::SimpleMath::Vector3 forward);

        void UpdateCamera(DirectX::SimpleMath::Vector3 characterPosition);

        Camera m_camera;
        DirectX::Mouse::ButtonStateTracker m_mouseButtonTracker;
        Physics::PhysicsEngine::CharacterID m_character;
    };
}