#pragma once

#include "Core/Rendering/DirectionalLight.h"
#include "Core/Parameters.h"
#include <directxtk12/SimpleMath.h>

namespace Gradient::GUI
{
    class RenderingWindow
    {
    public:


        RenderingWindow();

        void Draw();

        DirectX::XMFLOAT3 LightDirection = { 0, 0, 0 };
        DirectX::XMFLOAT3 LightColour = { 0, 0, 0 };
        float Irradiance;
        float AmbientIrradiance;

        float BloomExposure = 0.f;
        float BloomIntensity = 0.f;

        Params::Water Water;
        Params::PointLight PointLights[2];
    };
}