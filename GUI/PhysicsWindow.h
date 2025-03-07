#pragma once

namespace Gradient::GUI
{
    class PhysicsWindow
    {
    public:
        void Update();
        void Draw();
        void PauseSimulation();
        void UnpauseSimulation();

    private:
        float m_timeScale = 1.f;
        bool m_physicsPaused = false;
    };
}