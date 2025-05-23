//
// Game.h
//

#pragma once

#include <directxtk12/SimpleMath.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include <directxtk12/CommonStates.h>
#include <directxtk12/SpriteBatch.h>

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Core/FreeMoveCamera.h"
#include "Core/PlayerCharacter.h"
#include "Core/ECS/EntityManager.h"
#include "Core/Physics/PhysicsEngine.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/Rendering/RenderTexture.h"
#include "Core/Rendering/BloomProcessor.h"
#include "Core/Rendering/CubeMap.h"
#include "Core/Rendering/ProceduralMesh.h"
#include "Core/Parameters.h"
#include "GUI/PhysicsWindow.h"
#include "GUI/RenderingWindow.h"
#include "GUI/PerformanceWindow.h"
#include "GUI/EntityWindow.h"           
#include "GUI/ControlsWindow.h"

#include "Core/Rendering/Renderer.h"

using namespace DirectX::SimpleMath;

// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreateEntities();

    bool IsPlayingGame();
    void StartPlaying();
    void StartEditing();
    void TogglePlaying(float currentTime);
    void ToggleDebugMode(float currentTime);
    Gradient::Camera GetFrameCamera();

    float m_timeWhenToggleEnabled = 0.f;
    float m_timeWhenDebugToggleEnabled = 0.f;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;

    Gradient::FreeMoveCamera m_camera;
    std::unique_ptr<Gradient::PlayerCharacter> m_character;

    Gradient::Camera m_debugSavedCamera;
    bool m_debugMode = false;

    Gradient::GUI::PhysicsWindow m_physicsWindow;
    Gradient::GUI::RenderingWindow m_renderingWindow;
    Gradient::GUI::PerformanceWindow m_perfWindow;
    Gradient::GUI::EntityWindow m_entityWindow;
    Gradient::GUI::ControlsWindow m_controlsWindow;
                                                              
    std::unique_ptr<Gradient::Rendering::Renderer> m_renderer;
    std::unique_ptr<Gradient::Rendering::RenderTexture> m_tonemappedRenderTexture;
};
