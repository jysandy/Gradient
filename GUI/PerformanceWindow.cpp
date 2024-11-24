#include "pch.h"

#include "GUI/PerformanceWindow.h"
#include <imgui.h>

namespace Gradient::GUI
{
    void PerformanceWindow::Draw()
    {
        ImGui::Begin("Performance");

        ImGui::Text("FPS: %.2f", this->FPS);
        ImGui::Text("msPF: %.2f", 1000.f / this->FPS);

        ImGui::End();
    }
}