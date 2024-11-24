#pragma once

namespace Gradient::GUI
{
    class PerformanceWindow
    {
    public:
        PerformanceWindow() = default;

        void Draw();

        float FPS = 0.f;
    };
}