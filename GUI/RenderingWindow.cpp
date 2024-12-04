#include "pch.h"

#include "GUI/RenderingWindow.h"
#include <imgui.h>

namespace Gradient::GUI
{
    RenderingWindow::RenderingWindow() {}

    void RenderingWindow::Draw()
    {
        ImGui::Begin("Rendering");

        if (ImGui::TreeNode("Lighting"))
        {
            if (ImGui::TreeNode("Sun"))
            {
                ImGui::DragFloat3("Light direction",
                    &LightDirection.x,
                    0.001f,
                    -1.f,
                    1.f);
                ImGui::ColorEdit3("Light colour",
                    &LightColour.x,
                    ImGuiColorEditFlags_::ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("Light irradiance", &Irradiance, 0.f, 50.f);
                ImGui::SliderFloat("Ambient irradiance", &AmbientIrradiance, 0.f, 10.f);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Point lights"))
            {
                for (int i = 0; i < 2; i++)
                {
                    if (ImGui::TreeNode(("Point Light " + std::to_string(i + 1)).c_str()))
                    {
                        ImGui::ColorEdit3("Colour",
                            &PointLights[i].Colour.x,
                            ImGuiColorEditFlags_::ImGuiColorEditFlags_Float);
                        ImGui::SliderFloat("Irradiance", 
                            &PointLights[i].Irradiance,
                            0.f, 30.f);
                        ImGui::SliderFloat("Range",
                            &PointLights[i].MaxRange,
                            0.2f, 100.f);
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Water"))
        {
            ImGui::DragFloat("Near LOD distance",
                &Water.MinLod,
                0.5f,
                1.f,
                1000.f);

            ImGui::DragFloat("Far LOD distance",
                &Water.MaxLod,
                0.5f,
                1.f,
                1000.f);

            if (ImGui::TreeNode("Scattering"))
            {
                ImGui::SliderFloat("Thickness Height Scaling",
                    &Water.Scattering.ThicknessPower,
                    1.f,
                    10.f);
                ImGui::SliderFloat("Scattering Sharpness",
                    &Water.Scattering.Sharpness,
                    1.f,
                    10.f);
                ImGui::SliderFloat("Refractive Index",
                    &Water.Scattering.RefractiveIndex,
                    1.f,
                    5.f);
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Bloom"))
        {
            ImGui::SliderFloat("Exposure", &BloomExposure, 0.f, 20.f);
            ImGui::SliderFloat("Intensity", &BloomIntensity, 0.f, 1.f);
            ImGui::TreePop();
        }

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