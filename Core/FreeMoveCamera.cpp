#include "pch.h"

#include "Core/FreeMoveCamera.h"

#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include <algorithm>

using namespace DirectX::SimpleMath;

namespace Gradient
{
    void FreeMoveCamera::Update(DX::StepTimer const& timer)
    {
        if (!m_isActive) return;

        const float elapsedSeconds = timer.GetElapsedSeconds();
        const float sensitivity = 0.001f;

        auto mouseState = DirectX::Mouse::Get().GetState();

        if (mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
        {
            // Mouse movement
            auto yaw = -mouseState.x * sensitivity;
            auto pitch = -mouseState.y * sensitivity;
            m_camera.RotateYawPitch(yaw, pitch);
        }

        // Keyboard movement
        auto [right, up, forward] = m_camera.GetBasisVectors();
        auto backward = -forward;
        auto down = -up;
        auto left = -right;

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
        if (kb.Space)
        {
            translation += up;
        }
        if (kb.C)
        {
            translation += down;
        }

        translation.Normalize();

        const float speed = 10.f;
        auto oldPosition = m_camera.GetPosition();
        m_camera.SetPosition(oldPosition + (speed * elapsedSeconds) * translation);

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

    void FreeMoveCamera::SetPosition(const DirectX::SimpleMath::Vector3& position)
    {
        m_camera.SetPosition(position);
    }

    const Camera& FreeMoveCamera::GetCamera() const
    {
        return m_camera;
    }

    void FreeMoveCamera::SetAspectRatio(const float& aspectRatio)
    {
        m_camera.SetAspectRatio(aspectRatio);
    }

    void FreeMoveCamera::Activate()
    {
        m_isActive = true;
        auto& mouse = DirectX::Mouse::Get();
        mouse.SetMode(DirectX::Mouse::MODE_ABSOLUTE);
        m_mouseButtonTracker.Reset();
    }

    void FreeMoveCamera::Deactivate()
    {
        m_isActive = false;
    }

    bool FreeMoveCamera::IsActive()
    {
        return m_isActive;
    }
}