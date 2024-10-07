#include "pch.h"

#include "GUI/EntityWindow.h"
#include <imgui.h>

namespace Gradient::GUI
{
    void EntityWindow::Draw()
    {
        if (!m_enabled) return;

        auto entityManager = EntityManager::Get();

        ImGui::Begin("Entity");
        
        const auto& idSet = entityManager->GetIDs();
        std::vector<const char*> cstrings;
        for (const auto& id : idSet)
        {
            cstrings.push_back(id.c_str());
        }

        ImGui::Combo("Entity ID", 
            &m_currentEntityIdx, 
            cstrings.data(), 
            cstrings.size());

        if (m_currentEntityIdx != m_oldEntityIdx)
        {
            SyncEntityState();
            m_oldEntityIdx = m_currentEntityIdx;
        }

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
        entityManager->MutateEntity(GetCurrentEntityID(), m_mutator);
    }

    void EntityWindow::Disable()
    {
        m_enabled = false;
    }

    void EntityWindow::SyncEntityState()
    {
        auto entityManager = EntityManager::Get();
        auto e = entityManager->LookupEntity(GetCurrentEntityID());
        auto t = e->GetTranslation();
        auto r = e->GetRotationYawPitchRoll();

        m_translation[0] = t.x;
        m_translation[1] = t.y;
        m_translation[2] = t.z;

        m_rotationYawPitchRoll[0] = r.y;
        m_rotationYawPitchRoll[1] = r.x;
        m_rotationYawPitchRoll[2] = r.z;
    }

    std::string EntityWindow::GetCurrentEntityID()
    {
        auto entityManager = EntityManager::Get();
        const auto& idSet = entityManager->GetIDs();

        auto it = idSet.begin();
        std::advance(it, m_currentEntityIdx);
        return *it;
    }

    void EntityWindow::Enable()
    {
        if (m_enabled) return;

        SyncEntityState();
        m_enabled = true;
    }
}