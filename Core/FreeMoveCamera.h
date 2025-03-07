#pragma once

#include "pch.h"

#include "Core/Camera.h"

#include <directxtk12/Mouse.h>

namespace Gradient
{
    class FreeMoveCamera
    {
    public:

        void Update(DX::StepTimer const& timer);
        void SetPosition(DirectX::SimpleMath::Vector3 const&);
        void SetAspectRatio(const float& aspectRatio);
        const Camera& GetCamera() const;

        void Activate();
        void Deactivate();

        bool IsActive();

    private:
        bool m_isActive;

        Camera m_camera;

        DirectX::Mouse::ButtonStateTracker m_mouseButtonTracker;
    };
}