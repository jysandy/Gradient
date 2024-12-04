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
        void SyncEntityState();
        std::string GetCurrentEntityID();

        bool m_enabled = false;
        float m_translation[3];
        float m_rotationYawPitchRoll[3];

        int m_oldEntityIdx = 0;
        int m_currentEntityIdx = 0;

        DirectX::XMFLOAT3 m_emissiveRadiance = { 0, 0, 0 };

        std::function<void(Entity&)> m_mutator;
    };
}