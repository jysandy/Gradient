#include "pch.h"

#include "GUI/ControlsWindow.h"
#include <imgui.h>

namespace Gradient::GUI
{
    void ControlsWindow::Draw()
    {
        ImGui::Begin("Controls");

        ImGui::Text("Left click first to give the application keyboard control");
        ImGui::Text("Hold right click and use WASD to move the camera");
        ImGui::Text("Q and E to ascend and descend");
        ImGui::Text("Tab to freeze/unfreeze the culling frustum");
        ImGui::Text("P to toggle gameplay");

        ImGui::End();
    }
}