#include "pch.h"
#include "Camera.h"
#include <directxtk/Keyboard.h>
#include <directxtk/Mouse.h>
#include <algorithm>

using namespace DirectX::SimpleMath;

namespace Gradient
{
    Matrix Camera::GetViewMatrix()
    {
        Vector3 lookAt = m_position + m_direction;
        return Matrix::CreateLookAt(m_position,
            lookAt,
            Vector3{ 0, 1, 0 }
        );
    }

    Matrix Camera::GetProjectionMatrix()
    {
        return m_projectionMatrix;
    }

    void Camera::Update(DX::StepTimer const& timer)
    {
        const float elapsedSeconds = timer.GetElapsedSeconds();
        const float sensitivity = 0.001f;

        auto mouseState = DirectX::Mouse::Get().GetState();

        if (mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
        {
            // Mouse movement
            auto yAxis = Vector3::UnitY;
            auto forward = m_direction;
            auto right = forward.Cross(yAxis);
            right.Normalize();
            auto up = right.Cross(forward);
            up.Normalize();

            auto yaw = -mouseState.x * sensitivity;
            auto pitch = -mouseState.y * sensitivity;

            const auto maxPitchCosine = 0.99f;

            if (pitch < 0 && m_direction.Dot(yAxis) <= -maxPitchCosine)
                pitch = 0;

            if (pitch > 0 && m_direction.Dot(yAxis) >= maxPitchCosine)
                pitch = 0;

            auto rotationMatrix = Matrix::CreateFromAxisAngle(up, yaw)
                * Matrix::CreateFromAxisAngle(right, pitch);
            m_direction = Vector3::Transform(m_direction, rotationMatrix);
        }

        // Keyboard movement

        auto yAxis = Vector3{ 0, 1, 0 };
        auto forward = m_direction;
        auto backward = -forward;
        auto right = forward.Cross(yAxis);
        right.Normalize();
        auto left = -right;
        auto up = right.Cross(forward);
        up.Normalize();
        auto down = -up;

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

        const float speed = 4.f;
        m_position += (speed * elapsedSeconds) * translation;
         
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


    void Camera::CreateProjectionMatrix()
    {
        m_projectionMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            m_fieldOfViewRadians,
            m_aspectRatio,
            0.1f,
            500.f
        );
    }

    // These arent' thread safe.

    void Camera::SetFieldOfView(float const& fovRadians)
    {
        m_fieldOfViewRadians = fovRadians;
        CreateProjectionMatrix();
    }

    void Camera::SetAspectRatio(float const& aspectRatio)
    {
        m_aspectRatio = aspectRatio;
        CreateProjectionMatrix();
    }

    // Don't know if this is thread safe or not.
    void Camera::SetPosition(Vector3 const& pos)
    {
        m_position = pos;
    }
}