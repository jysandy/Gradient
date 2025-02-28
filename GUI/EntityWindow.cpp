#include "pch.h"

#include "GUI/EntityWindow.h"
#include "Core/ECS/EntityManager.h"
#include "Core/Physics/PhysicsEngine.h"
#include "Core/ECS/Components/NameTagComponent.h"
#include "Core/ECS/Components/DrawableComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/MaterialComponent.h"
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/ECS/Components/PointLightComponent.h"
#include <imgui.h>

namespace Gradient::GUI
{
    void EntityWindow::Draw()
    {
        using namespace Gradient::ECS::Components;
        using DirectX::SimpleMath::Vector3;

        auto em = Gradient::EntityManager::Get();

        ImGui::Begin("Entities");

        std::string preview;

        if (m_selectedEntity)
            preview = em->Registry.get<NameTagComponent>(m_selectedEntity.value()).Name;
        else
            preview = "Choose an Entity";

        if (ImGui::BeginCombo("##", preview.c_str(), 0))
        {
            auto view = em->Registry.view<NameTagComponent>();
            for (auto entity : view)
            {
                auto [nameTag] = view.get(entity);

                const bool isSelected = (entity == m_selectedEntity);

                if (ImGui::Selectable(nameTag.Name.c_str(), isSelected)
                    && m_selectedEntity != entity)
                {
                    m_selectedEntity = entity;
                    SyncTransformState();
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (m_selectedEntity)
        {
            // TODO: Render the GUI here.

            auto [nameTag, transform]
                = em->Registry.get<NameTagComponent,
                TransformComponent>(m_selectedEntity.value());

            auto name = nameTag.Name;

            if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BeginDisabled(!m_transformEditingEnabled);

                if (!m_transformEditingEnabled)
                {
                    ImGui::Text("Out of date! Disable physics to edit transform");
                }

                if (ImGui::DragFloat3(("Translation##" + name).c_str(),
                    m_translation,
                    0.1f))
                {
                    em->SetTranslation(m_selectedEntity.value(),
                        Vector3{ m_translation[0],
                       m_translation[1],
                       m_translation[2]
                        });
                }

                if (ImGui::DragFloat3(("Rotation yaw/pitch/roll##" + name).c_str(),
                    m_rotationYawPitchRoll, 0.1f,
                    -DirectX::XM_2PI, DirectX::XM_2PI))
                {
                    em->SetRotation(m_selectedEntity.value(),
                        m_rotationYawPitchRoll[0],
                        m_rotationYawPitchRoll[1],
                        m_rotationYawPitchRoll[2]);
                }

                ImGui::EndDisabled();

                ImGui::TreePop();
            }
        }

        if (Gradient::Physics::PhysicsEngine::Get()->IsPaused())
        {
            EnableTransformEditing();
        }
        else
        {
            // We could sync state here every frame so the disabled values 
            // update, but that has a heavy impact on performance.
            // TODO: sync at a low tickrate instead?
            DisableTransformEditing();
        }

        ImGui::End();
    }

    void EntityWindow::SyncTransformState()
    {
        if (!m_selectedEntity) return;

        auto em = Gradient::EntityManager::Get();

        auto t = em->GetTranslation(m_selectedEntity.value());
        auto r = em->GetRotationYawPitchRoll(m_selectedEntity.value());

        m_translation[0] = t.x;
        m_translation[1] = t.y;
        m_translation[2] = t.z;

        m_rotationYawPitchRoll[0] = r.y;
        m_rotationYawPitchRoll[1] = r.x;
        m_rotationYawPitchRoll[2] = r.z;
    }

    void EntityWindow::EnableTransformEditing()
    {
        if (m_transformEditingEnabled) return;

        SyncTransformState();
        m_transformEditingEnabled = true;
    }

    void EntityWindow::DisableTransformEditing()
    {
        m_transformEditingEnabled = false;
    }
}