#include "pch.h"
#include "Camera.h"
#include "Math.h"
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include <algorithm>

using namespace DirectX::SimpleMath;

namespace Gradient
{
    Matrix Camera::GetViewMatrix() const
    {
        Vector3 lookAt = m_position + m_direction;
        return Matrix::CreateLookAt(m_position,
            lookAt,
            Vector3{ 0, 1, 0 }
        );
    }

    Matrix Camera::GetProjectionMatrix() const
    {
        return m_projectionMatrix;
    }

    void Camera::CreateProjectionMatrix()
    {
        m_projectionMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            m_fieldOfViewRadians,
            m_aspectRatio,
            0.1f,
            300.f
        );

        // A shorter view distance is used for shadows.
        m_shadowProjectionMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            m_fieldOfViewRadians,
            m_aspectRatio,
            0.1f,
            70.f
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

    void Camera::RotateYawPitch(float yaw, float pitch)
    {
        const auto maxPitchCosine = 0.99f;

        if (pitch < 0 && m_direction.Dot(Vector3::UnitY) <= -maxPitchCosine)
            pitch = 0;

        if (pitch > 0 && m_direction.Dot(Vector3::UnitY) >= maxPitchCosine)
            pitch = 0;

        auto [right, up, forward] = GetBasisVectors();

        auto rotationQuat = 
            Quaternion::CreateFromAxisAngle(Vector3::UnitY, yaw)
            * Quaternion::CreateFromAxisAngle(right, pitch);

        m_direction = Vector3::Transform(m_direction, rotationQuat);
        m_direction.Normalize();
    }

    Vector3 Camera::GetPosition() const
    {
        return m_position;
    }

    Vector3 Camera::GetDirection() const
    {
        return m_direction;
    }

    std::tuple<DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3>
        Camera::GetBasisVectors() const
    {
        auto forward = m_direction;
        auto right = forward.Cross(Vector3::UnitY);
        right.Normalize();
        auto up = right.Cross(forward);
        up.Normalize();

        return std::make_tuple(right, up, forward);
    }

    DirectX::BoundingFrustum Camera::GetFrustum() const
    {
        return Math::MakeFrustum(GetViewMatrix(), m_projectionMatrix);
    }

    DirectX::BoundingFrustum Camera::GetShadowFrustum() const
    {
        return Math::MakeFrustum(GetViewMatrix(), m_shadowProjectionMatrix);
    }
}