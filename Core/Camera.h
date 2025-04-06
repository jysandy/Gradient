#pragma once

#include "StepTimer.h"
#include <directxtk12/SimpleMath.h>
#include <tuple>

namespace Gradient
{
    class Camera
    {
    private:
        constexpr static float DEFAULT_FOV = DirectX::XM_PI / 3.f;
        constexpr static float DEFAULT_ASPECT_RATIO = 1920.f / 1080.f;
        float m_fieldOfViewRadians;
        float m_aspectRatio;
        DirectX::SimpleMath::Vector3 m_position;
        DirectX::SimpleMath::Vector3 m_direction;

        DirectX::SimpleMath::Matrix m_projectionMatrix;
        
        void CreateProjectionMatrix();

    public:
        Camera() : m_fieldOfViewRadians(DEFAULT_FOV),
            m_aspectRatio(DEFAULT_ASPECT_RATIO),
            m_position(DirectX::SimpleMath::Vector3{ 0, 0, 5 }),
            m_direction(-DirectX::SimpleMath::Vector3::UnitZ)
        {
            CreateProjectionMatrix();
        }

        virtual ~Camera() {}

        DirectX::SimpleMath::Matrix GetViewMatrix() const;
        DirectX::SimpleMath::Matrix GetProjectionMatrix() const;
        DirectX::BoundingFrustum GetFrustum() const;

        void SetFieldOfView(float const& fovRadians);
        void SetAspectRatio(float const& aspectRatio);
        void SetPosition(DirectX::SimpleMath::Vector3 const&);
        void RotateYawPitch(float yaw, float pitch);
        DirectX::SimpleMath::Vector3 GetPosition() const;
        DirectX::SimpleMath::Vector3 GetDirection() const;

        // right, up, forward
        std::tuple<DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3>
            GetBasisVectors() const;
    };
}