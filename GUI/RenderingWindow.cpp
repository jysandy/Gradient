#include "pch.h"

#include "GUI/RenderingWindow.h"
#include <imgui.h>

namespace Gradient::GUI
{
    RenderingWindow::RenderingWindow(){}

    void RenderingWindow::Draw()
    {
        ImGui::Begin("Rendering");
        ImGui::DragFloat3("Light direction",
            &LightDirection.x,
            0.001f,
            -1.f,
            1.f);
        ImGui::ColorEdit3("Light colour",
            &LightColour.x);
        ImGui::SliderFloat("Light irradiance", &Irradiance, 0.f, 50.f);
        ImGui::SliderFloat("Ambient irradiance", &AmbientIrradiance, 0.f, 10.f);
        ImGui::End();
    }    

    void RenderingWindow::SetLinearLightColour(DirectX::SimpleMath::Color c)
    {
        LightColour.x = pow(c.R(), 1.f / 2.2f);
        LightColour.y = pow(c.G(), 1.f / 2.2f);
        LightColour.z = pow(c.B(), 1.f / 2.2f);
    }

    DirectX::SimpleMath::Color RenderingWindow::GetLinearLightColour()
    {
        auto lightColour = DirectX::SimpleMath::Color(
            pow(LightColour.x, 2.2),
            pow(LightColour.y, 2.2),
            pow(LightColour.z, 2.2)
        );

        return lightColour;
    }
}