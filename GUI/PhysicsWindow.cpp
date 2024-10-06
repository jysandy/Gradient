#include "pch.h"
#include "GUI/PhysicsWindow.h"
#include "Core/Physics/PhysicsEngine.h"
#include <imgui.h>

namespace Gradient::GUI
{
    void PhysicsWindow::Update()
    {
        auto physicsEngine = Gradient::Physics::PhysicsEngine::Get();
        physicsEngine->SetTimeScale(m_timeScale);
        if (m_physicsPaused)
        {
            physicsEngine->PauseSimulation();
        }
        else
        {
            physicsEngine->UnpauseSimulation();
        }
    }

    void PhysicsWindow::Draw()
    {
        ImGui::Begin("Physics controls");
        ImGui::Checkbox("Physics paused", &m_physicsPaused);
        ImGui::SliderFloat("Time scale", &m_timeScale, 0.1f, 1.f);
        ImGui::End();
    }
}