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

        ImGui::DragFloat3(("Translation##" + GetCurrentEntityID()).c_str(), 
            m_translation, 
            0.1f);
        ImGui::DragFloat3(("Rotation yaw/pitch/roll##" + GetCurrentEntityID()).c_str(),
            m_rotationYawPitchRoll, 0.1f, -DirectX::XM_2PI, DirectX::XM_2PI);
        ImGui::DragFloat3(("Emissive Radiance##" + GetCurrentEntityID()).c_str(),
            &m_emissiveRadiance.x,
            0.1f, 0.f, 50.f);
        ImGui::Checkbox(("Casts Shadows##" + GetCurrentEntityID()).c_str(),
            &m_castsShadows);
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
            e.Material.EmissiveRadiance = m_emissiveRadiance;
            e.CastsShadows = m_castsShadows;
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

        m_emissiveRadiance = e->Material.EmissiveRadiance;
        m_castsShadows = e->CastsShadows;
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