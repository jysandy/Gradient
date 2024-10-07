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
        ImGui::End();
    }

    EntityWindow::EntityWindow() : 
        m_translation{0.f, 0.f, 0.f},
        m_rotation{0.f, 0.f, 0.f}
    {
        m_mutator = [this](Entity& e) {
            e.SetTranslation(DirectX::SimpleMath::Vector3(
                m_translation[0],
                m_translation[1],
                m_translation[2]
            ));
            };
    }

    void EntityWindow::Update(EntityManager& entityManager)
    {
        if (!m_enabled) return;

        entityManager.MutateEntity(m_entityID, m_mutator);
    }

    void EntityWindow::Disable()
    {
        m_enabled = false;
    }

    void EntityWindow::Enable(EntityManager& entityManager)
    {
        if (m_enabled) return;

        auto e = entityManager.LookupEntity(m_entityID);
        auto t = e->GetTranslation();

        m_translation[0] = t.x;
        m_translation[1] = t.y;
        m_translation[2] = t.z;

        m_enabled = true;
    }
}