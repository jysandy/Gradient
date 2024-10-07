#pragma once

#include "Core/EntityManager.h"

namespace Gradient::GUI
{
    class EntityWindow
    {
    public:
        EntityWindow();

        void Update();
        void Draw();

        void Enable();
        void Disable();

    private:
        bool m_enabled = false;
        float m_translation[3];
        float m_rotation[3];

        std::function<void(Entity&)> m_mutator;

        const std::string m_entityID = "sphere1";
    };
}