#pragma once

#include "pch.h"

#include <entt/entt.hpp>
#include <optional>

namespace Gradient::GUI
{
    class EntityWindow
    {
    public:
        void Draw();

    private:
        void SyncTransformState();
        void EnableTransformEditing();
        void DisableTransformEditing();

        std::optional<entt::entity> m_selectedEntity;

        float m_translation[3];
        float m_rotationYawPitchRoll[3];

        bool m_transformEditingEnabled = false;
    };
}