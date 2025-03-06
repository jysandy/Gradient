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

    Vector3 PlayerCharacter::GetMovementInput(
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

        translation.Normalize();

        return translation;
    }

    void PlayerCharacter::Update(const DX::StepTimer& timer)
    {
        auto physics = Physics::PhysicsEngine::Get();

        Vector3 characterPosition = Physics::FromJolt(
            physics->GetCharacterPosition(m_character));

        UpdateCamera(characterPosition);
    }

    void PlayerCharacter::PhysicsUpdate(const float& deltaTime,
        JPH::Ref<JPH::CharacterVirtual> character,
        JPH::PhysicsSystem* system)
    {
        // TODO: Make this thread safe
        auto [right, up, forward] = m_camera.GetBasisVectors();

        auto input = GetMovementInput(right, forward);

        const float speed = 5.f;
        JPH::Vec3 newVelocity = JPH::Vec3::sZero();

        auto currentVelocity = character->GetLinearVelocity();

        newVelocity += speed * Physics::ToJolt(input);

        if (character->IsSupported())
        {
            // Don't add gravity    
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
        m_camera.SetPosition(characterPosition + Vector3::UnitY);

        const float sensitivity = 0.001f;

        auto mouseState = DirectX::Mouse::Get().GetState();

        if (mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
        {
            // Mouse movement
            auto yaw = -mouseState.x * sensitivity;
            auto pitch = -mouseState.y * sensitivity;
            m_camera.RotateYawPitch(yaw, pitch);
        }

        m_mouseButtonTracker.Update(mouseState);

        auto& mouse = DirectX::Mouse::Get();
        if (m_mouseButtonTracker.leftButton == DirectX::Mouse::ButtonStateTracker::ButtonState::PRESSED)
        {
            mouse.SetMode(DirectX::Mouse::MODE_RELATIVE);
        }
        else if (m_mouseButtonTracker.leftButton == DirectX::Mouse::ButtonStateTracker::ButtonState::RELEASED)
        {
            mouse.SetMode(DirectX::Mouse::MODE_ABSOLUTE);
        }
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
}