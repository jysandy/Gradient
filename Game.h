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
#include "Core/Camera.h"
#include "Core/Entity.h"
#include "Core/EntityManager.h"
#include "Core/Physics/PhysicsEngine.h"
#include "Core/Pipelines/PBRPipeline.h"
#include "Core/Pipelines/SkyDomePipeline.h"
#include "Core/Pipelines/WaterPipeline.h"
#include "Core/Pipelines/HeightmapPipeline.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/Rendering/RenderTexture.h"
#include "Core/Rendering/BloomProcessor.h"
#include "Core/Rendering/CubeMap.h"
#include "Core/Rendering/GeometricPrimitive.h"
#include "Core/Parameters.h"
#include "GUI/PhysicsWindow.h"
#include "GUI/EntityWindow.h"
#include "GUI/RenderingWindow.h"
#include "GUI/PerformanceWindow.h"

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

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreateEntities();
    std::vector<Gradient::Params::PointLight> PointLightParams();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;

    std::unique_ptr<Gradient::Rendering::RenderTexture> m_multisampledRenderTexture;
    std::unique_ptr<Gradient::Rendering::RenderTexture> m_tonemappedRenderTexture;
    std::unique_ptr<Gradient::Rendering::BloomProcessor> m_bloomProcessor;

    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;
    std::shared_ptr<DirectX::CommonStates> m_states;

    Gradient::Camera m_camera;
    
    std::unique_ptr<Gradient::Pipelines::PBRPipeline> m_pbrPipeline;
    std::unique_ptr<Gradient::Pipelines::WaterPipeline> m_waterPipeline;
    std::unique_ptr<Gradient::Pipelines::HeightmapPipeline> m_heightmapPipeline;

    Gradient::GUI::PhysicsWindow m_physicsWindow;
    Gradient::GUI::EntityWindow m_entityWindow;
    Gradient::GUI::RenderingWindow m_renderingWindow;
    Gradient::GUI::PerformanceWindow m_perfWindow;

    std::unique_ptr<Gradient::Rendering::DirectionalLight> m_dLight;
    std::vector<Gradient::Rendering::PointLight> m_pointLights;
    std::unique_ptr<Gradient::Rendering::DepthCubeArray> m_shadowCubeArray;

    std::unique_ptr<Gradient::Rendering::GeometricPrimitive> m_sky;
    std::unique_ptr<Gradient::Pipelines::SkyDomePipeline> m_skyDomePipeline;
    std::unique_ptr<Gradient::Rendering::CubeMap> m_environmentMap;
    
    std::unique_ptr<Gradient::Rendering::TextureDrawer> m_tonemapper;
};
