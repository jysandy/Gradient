#pragma once

#include "Core/Rendering/DirectionalLight.h"
#include <directxtk/SimpleMath.h>

namespace Gradient::GUI
{
    class RenderingWindow
    {
    public:
        RenderingWindow();

        void Draw();

        // These methods are necessary because ImGUI doesn't differentiate 
        // between sRGB vs linear colours.

        void SetLinearLightColour(DirectX::SimpleMath::Color c);
        DirectX::SimpleMath::Color GetLinearLightColour();

        DirectX::XMFLOAT3 LightDirection = { 0, 0, 0 };
        DirectX::XMFLOAT3 LightColour = { 0, 0, 0 };
        float Irradiance;
        float AmbientIrradiance;
    };
}