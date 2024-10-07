#include "pch.h"

#include "GUI/EntityWindow.h"
#include <imgui.h>

namespace Gradient::GUI
{
    void EntityWindow::Draw()
    {
        if (!m_enabled) return;

        ImGui::Begin("Entity");
        ImGui::InputFloat3("Translation", m_translation);
        ImGui::SliderFloat3("Rotation yaw/pitch/roll", m_rotationYawPitchRoll, -DirectX::XM_2PI, DirectX::XM_2PI);
        ImGui::End();
    }

    EntityWindow::EntityWindow() : 
        m_translation{0.f, 0.f, 0.f},
        m_rotationYawPitchRoll{0.f, 0.f, 0.f}
    {
        m_mutator = [this](Entity& e) {
            e.SetTranslation(DirectX::SimpleMath::Vector3(
                m_translation[0],
                m_translation[1],
                m_translation[2]
            ));

            e.SetRotation(m_rotationYawPitchRoll[0], m_rotationYawPitchRoll[1], m_rotationYawPitchRoll[2]);
            };
    }

    void EntityWindow::Update()
    {
        if (!m_enabled) return;

        auto entityManager = EntityManager::Get();
        entityManager->MutateEntity(m_entityID, m_mutator);
    }

    void EntityWindow::Disable()
    {
        m_enabled = false;
    }

    void EntityWindow::Enable()
    {
        if (m_enabled) return;

        auto entityManager = EntityManager::Get();
        auto e = entityManager->LookupEntity(m_entityID);
        auto t = e->GetTranslation();
        auto r = e->GetRotationYawPitchRoll();

        m_translation[0] = t.x;
        m_translation[1] = t.y;
        m_translation[2] = t.z;

        m_rotationYawPitchRoll[0] = r.y;
        m_rotationYawPitchRoll[1] = r.x;
        m_rotationYawPitchRoll[2] = r.z;

        m_enabled = true;
    }
}