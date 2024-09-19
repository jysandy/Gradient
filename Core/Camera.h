#pragma once

#include "StepTimer.h"
#include <directxtk/SimpleMath.h>
#include <directxtk/Mouse.h>

namespace Gradient
{
    class Camera
    {
    private:
        constexpr static float DEFAULT_FOV = DirectX::XM_PI / 4.f;
        constexpr static float DEFAULT_ASPECT_RATIO = 16.f / 9.f;
        float m_fieldOfViewRadians;
        float m_aspectRatio;
        DirectX::SimpleMath::Vector3 m_position;
        DirectX::SimpleMath::Vector3 m_direction;

        DirectX::SimpleMath::Matrix m_projectionMatrix;
        DirectX::Mouse::ButtonStateTracker m_mouseButtonTracker;
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

        void Update(DX::StepTimer const& timer);

        DirectX::SimpleMath::Matrix GetViewMatrix();
        DirectX::SimpleMath::Matrix GetProjectionMatrix();

        void SetFieldOfView(float const& fovRadians);
        void SetAspectRatio(float const& aspectRatio);
    };
}