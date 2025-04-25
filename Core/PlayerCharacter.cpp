#include "pch.h"


#include "Core/PlayerCharacter.h"
#include "Core/Physics/Conversions.h"
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>


using namespace DirectX::SimpleMath;

namespace Gradient
{
    PlayerCharacter::PlayerCharacter()
    {
        JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();

        float cylinderHalfHeight = 1.f;
        float sphereRadius = 0.5f;

        auto shape = JPH::RotatedTranslatedShapeSettings(
            { 0, cylinderHalfHeight + sphereRadius, 0 },
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(cylinderHalfHeight, sphereRadius))
            .Create().Get();

        settings->mShape = shape;

        auto physics = Physics::PhysicsEngine::Get();
        m_character = physics->CreateCharacter(settings,
            [this](const float& deltaTime, 
                JPH::Ref<JPH::CharacterVirtual> character,
                JPH::PhysicsSystem* system)
            {
                this->PhysicsUpdate(deltaTime, character, system);
            });
    }

    std::tuple<Vector3, bool> PlayerCharacter::GetMovementInput(
        Vector3 right,
        Vector3 forward)
    {
        // Project right and forward into the XZ plane.
        right.y = 0;
        right.Normalize();
        forward.y = 0;
        forward.Normalize();

        auto left = -right;
        auto backward = -forward;

        auto translation = Vector3::Zero;
        auto kb = DirectX::Keyboard::Get().GetState();

        auto jumped = false;

        if (kb.W)
        {
            translation += forward;
        }
        if (kb.A)
        {
            translation += left;
        }
        if (kb.S)
        {
            translation += backward;
        }
        if (kb.D)
        {
            translation += right;
        }
        if (kb.Space)
        {
            jumped = true;
        }

        translation.Normalize();

        return std::make_tuple(translation, jumped);
    }

    void PlayerCharacter::Update(const DX::StepTimer& timer)
    {
        if (!m_isActive.test()) return;

        auto physics = Physics::PhysicsEngine::Get();

        Vector3 characterPosition = Physics::FromJolt(
            physics->GetCharacterPosition(m_character));

        UpdateCamera(characterPosition);
    }

    void PlayerCharacter::PhysicsUpdate(const float& deltaTime,
        JPH::Ref<JPH::CharacterVirtual> character,
        JPH::PhysicsSystem* system)
    {
        if (!m_isActive.test()) return;

        auto [right, up, forward] = GetBasisVectorsThreadSafe();

        auto [input, jumped] = GetMovementInput(right, forward);

        const float speed = 5.f;
        const float jumpSpeed = 5.f;
        JPH::Vec3 newVelocity = JPH::Vec3::sZero();

        auto currentVelocity = character->GetLinearVelocity();

        newVelocity += speed * Physics::ToJolt(input);

        auto groundNormal = Physics::FromJolt(character->GetGroundNormal());

        if (character->IsSupported() && groundNormal.Dot(Vector3::UnitY) > 0.2f)
        {
            if (jumped)
            {
                newVelocity += {0, jumpSpeed, 0};
            }
        }
        else
        {
            newVelocity += JPH::Vec3{ 0, currentVelocity.GetY(), 0 } +
                deltaTime * system->GetGravity();
        }

        character->SetLinearVelocity(newVelocity);
    }

    void PlayerCharacter::UpdateCamera(Vector3 characterPosition)
    {
        const float sensitivity = 0.001f;

        auto mouseState = DirectX::Mouse::Get().GetState();

        {
            std::unique_lock lock(m_cameraMutex);

            if (mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
            {
                // Mouse movement
                auto yaw = -mouseState.x * sensitivity;
                auto pitch = -mouseState.y * sensitivity;
                m_camera.RotateYawPitch(yaw, pitch);
            }
            m_camera.SetPosition(characterPosition + Vector3::UnitY);
        }

        m_mouseButtonTracker.Update(mouseState);
    }

    void PlayerCharacter::SetPosition(const Vector3& position)
    {
        auto physics = Physics::PhysicsEngine::Get();
        physics->MutateCharacter(m_character,
            [position](JPH::Ref<JPH::CharacterVirtual> character)
            {
                character->SetPosition(Physics::ToJolt(position));
            });

        m_camera.SetPosition(position + Vector3::UnitY);
    }

    void PlayerCharacter::SetAspectRatio(const float& aspectRatio)
    {
        m_camera.SetAspectRatio(aspectRatio);
    }

    const Camera& PlayerCharacter::GetCamera() const
    {
        return m_camera;
    }

    void PlayerCharacter::Activate()
    {
        m_isActive.test_and_set();
        auto& mouse = DirectX::Mouse::Get();
        mouse.SetMode(DirectX::Mouse::MODE_RELATIVE);
        m_mouseButtonTracker.Reset();
    }

    void PlayerCharacter::Deactivate()
    {
        m_isActive.clear();
        auto& mouse = DirectX::Mouse::Get();
        mouse.SetMode(DirectX::Mouse::MODE_ABSOLUTE);
    }

    bool PlayerCharacter::IsActive()
    {
        return m_isActive.test();
    }

    std::tuple<Vector3,
        Vector3,
        Vector3> PlayerCharacter::GetBasisVectorsThreadSafe()
    {
        std::shared_lock lock(m_cameraMutex);

        return m_camera.GetBasisVectors();
    }
}