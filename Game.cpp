//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "directxtk12/Keyboard.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/TextureManager.h"
#include "Core/BufferManager.h"
#include "Core/Rendering/TextureDrawer.h"
#include "Core/Rendering/ProceduralMesh.h"
#include "Core/Rendering/LSystem.h"
#include "Core/Parameters.h"
#include "Core/Scene.h"
#include <directxtk12/SimpleMath.h>

#include <imgui.h>
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx12.h"
#include "Core/ReadData.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2,
        D3D_FEATURE_LEVEL_12_2,
        DX::DeviceResources::c_AllowTearing
    );
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

void Game::Initialize(HWND window, int width, int height)
{
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
    m_mouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

    Gradient::BufferManager::Initialize();
    Gradient::Physics::PhysicsEngine::Initialize();
    m_deviceResources->SetWindow(window, width, height);

    m_character = std::make_unique<Gradient::PlayerCharacter>();
    m_character->SetPosition(Vector3{ 0, 30, 25 });
    m_camera.SetPosition(Vector3{ 0, 30, 25 });

    StartEditing();

    m_renderer = std::make_unique<Gradient::Rendering::Renderer>();

    m_deviceResources->CreateDeviceResources();

    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    auto cq = m_deviceResources->GetCommandQueue();

    Gradient::Physics::PhysicsEngine::Get()->StartSimulation();
}

bool Game::IsPlayingGame()
{
    return !m_camera.IsActive();
}

void Game::StartPlaying()
{
    m_debugMode = false;
    m_camera.Deactivate();
    m_character->Activate();
    m_physicsWindow.UnpauseSimulation();
}

void Game::StartEditing()
{
    m_debugMode = false;
    m_character->Deactivate();
    m_camera.Activate();
    m_physicsWindow.PauseSimulation();
}

void Game::TogglePlaying(float currentTime)
{
    if (currentTime < m_timeWhenToggleEnabled) return;

    DirectX::Keyboard::Get().Reset();

    if (IsPlayingGame())
    {
        StartEditing();
    }
    else
    {
        StartPlaying();
    }

    m_timeWhenToggleEnabled = currentTime + 0.5;
}

Gradient::Camera Game::GetFrameCamera()
{
    if (!IsPlayingGame())
    {
        return m_camera.GetCamera();
    }
    else
    {
        return m_character->GetCamera();
    }
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    if (IsPlayingGame())
    {
        m_character->Update(timer);

    }
    else
    {
        m_camera.Update(timer);
    }

    auto entityManager = Gradient::EntityManager::Get();

    entityManager->UpdateAll(timer);

    m_physicsWindow.Update();

    m_perfWindow.FPS = timer.GetFramesPerSecond();

    m_renderer->DirectionalLight->SetLightDirection(m_renderingWindow.LightDirection);
    m_renderer->DirectionalLight->SetColour(DirectX::SimpleMath::Color(m_renderingWindow.LightColour));
    m_renderer->DirectionalLight->SetIrradiance(m_renderingWindow.Irradiance);

    // TODO: Move this into the window
    auto lightView = entityManager->Registry.view<Gradient::ECS::Components::PointLightComponent>();
    int i = 0;
    for (auto entity : lightView)
    {
        auto [light] = lightView.get(entity);
        light.PointLight.SetParams(m_renderingWindow.PointLights[i]);
        i++;
    }

    m_renderer->SkyDomePipeline->SetAmbientIrradiance(m_renderingWindow.AmbientIrradiance);
    auto totalSeconds = m_timer.GetTotalSeconds();

    m_renderer->WaterPipeline->SetTotalTime(totalSeconds);
    m_renderer->WaterPipeline->SetWaterParams(m_renderingWindow.Water);
    m_renderer->BloomProcessor->SetExposure(m_renderingWindow.BloomExposure);
    m_renderer->BloomProcessor->SetIntensity(m_renderingWindow.BloomIntensity);
    m_renderer->BillboardPipeline->TotalTimeSeconds = totalSeconds;

    auto kb = DirectX::Keyboard::Get().GetState();
    if (kb.OemTilde)
    {
        TogglePlaying(timer.GetTotalSeconds());
    }
    else if (kb.Tab)
    {
        ToggleDebugMode(timer.GetTotalSeconds());
    }
}

void Game::ToggleDebugMode(float currentTime)
{
    if (currentTime < m_timeWhenDebugToggleEnabled) return;

    DirectX::Keyboard::Get().Reset();

    if (!IsPlayingGame())
    {
        if (m_debugMode)
        {
            m_debugMode = false;
        }
        else
        {
            m_debugSavedCamera = m_camera.GetCamera();
            m_debugMode = true;
        }
    }

    m_timeWhenDebugToggleEnabled = currentTime + 0.5;
}

#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_COPY_DEST);

    auto cl = m_deviceResources->GetCommandList();

    auto frameCamera = GetFrameCamera();

    Gradient::Camera* cullingCamera = &frameCamera;

    if (!IsPlayingGame() && m_debugMode)
    {
        cullingCamera = &m_debugSavedCamera;
    }

    m_renderer->Render(cl,
        m_deviceResources->GetScreenViewport(),
        &frameCamera,
        cullingCamera,
        m_tonemappedRenderTexture.get());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_tonemappedRenderTexture->SetDepthAndRT(cl);

    m_perfWindow.Draw();

    if (!IsPlayingGame())
    {
        m_physicsWindow.Draw();
        m_renderingWindow.Draw();
        m_entityWindow.Draw();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
        cl);

    // This should already be in the COPY_DEST state
    auto presentResource = m_deviceResources->GetRenderTarget();
    auto tonemappedBarrierResource =
        m_tonemappedRenderTexture->GetSingleSampledBarrierResource();
    tonemappedBarrierResource->Transition(cl, D3D12_RESOURCE_STATE_COPY_SOURCE);

    cl->CopyResource(presentResource,
        tonemappedBarrierResource->Get());

    // Show the new frame.
    m_deviceResources->Present(D3D12_RESOURCE_STATE_COPY_DEST);

    auto gmm = Gradient::GraphicsMemoryManager::Get();
    gmm->Commit(m_deviceResources->GetCommandQueue());
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

Game::~Game()
{
    Gradient::Physics::PhysicsEngine::Shutdown();
    Gradient::TextureManager::Shutdown();
    Gradient::EntityManager::Shutdown();
    Gradient::Rendering::TextureDrawer::Shutdown();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    Gradient::GraphicsMemoryManager::Shutdown();
    m_deviceResources.reset();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1920;
    height = 1080;
}
#pragma endregion

void Game::CreateEntities()
{
    using namespace Gradient;
    using namespace Gradient::ECS::Components;

    auto entityManager = EntityManager::Get();
    auto textureManager = TextureManager::Get();
    auto device = m_deviceResources->GetD3DDevice();
    auto cq = m_deviceResources->GetCommandQueue();
    auto cl = m_deviceResources->GetCommandList();

#pragma region Textures
    // TODO: display a loading screen while loading all these textures
    // TODO: load these in a single upload batch
    // TODO: cut down on the texture size, these are way too big

    textureManager->LoadDDS(device, cq,
        "crate",
        L"Assets\\Wood_Crate_001_basecolor.dds");
    textureManager->LoadDDS(device, cq,
        "crateNormal",
        L"Assets\\Wood_Crate_001_normal.dds");
    textureManager->LoadDDS(device, cq,
        "crateRoughness",
        L"Assets\\Wood_Crate_001_roughness.dds");
    textureManager->LoadDDS(device, cq,
        "crateAO",
        L"Assets\\Wood_Crate_001_ambientOcclusion.dds");

    textureManager->LoadDDS(device, cq,
        "tilesAlbedo",
        L"Assets\\TilesDiffuse.dds");
    textureManager->LoadDDS(device, cq,
        "tilesNormal",
        L"Assets\\TilesNormal.dds");
    textureManager->LoadDDS(device, cq,
        "tilesAO",
        L"Assets\\TilesAO.dds");
    textureManager->LoadDDS(device, cq,
        "tilesMetalness",
        L"Assets\\TilesMetalness.dds");
    textureManager->LoadDDS(device, cq,
        "tilesRoughness",
        L"Assets\\TilesRoughness.dds");

    textureManager->LoadDDS(device, cq,
        "tiles06Albedo",
        L"Assets\\Tiles_Decorative_06_basecolor.dds");
    textureManager->LoadDDS(device, cq,
        "tiles06Normal",
        L"Assets\\Tiles_Decorative_06_normal.dds");
    textureManager->LoadDDS(device, cq,
        "tiles06AO",
        L"Assets\\Tiles_Decorative_06_ambientocclusion.dds");
    textureManager->LoadDDS(device, cq,
        "tiles06Metalness",
        L"Assets\\Tiles_Decorative_06_metallic.dds");
    textureManager->LoadDDS(device, cq,
        "tiles06Roughness",
        L"Assets\\Tiles_Decorative_06_roughness.dds");

    textureManager->LoadDDS(device, cq,
        "metal01Albedo",
        L"Assets\\Metal_Floor_01_basecolor.dds");
    textureManager->LoadDDS(device, cq,
        "metal01Normal",
        L"Assets\\Metal_Floor_01_normal.dds");
    textureManager->LoadDDS(device, cq,
        "metal01AO",
        L"Assets\\Metal_Floor_01_ambientocclusion.dds");
    textureManager->LoadDDS(device, cq,
        "metal01Metalness",
        L"Assets\\Metal_Floor_01_metallic.dds");
    textureManager->LoadDDS(device, cq,
        "metal01Roughness",
        L"Assets\\Metal_Floor_01_roughness.dds");

    textureManager->LoadDDS(device, cq,
        "metalSAlbedo",
        L"Assets\\Metal_Semirough_01_basecolor.dds");
    textureManager->LoadDDS(device, cq,
        "metalSNormal",
        L"Assets\\Metal_Semirough_01_normal.dds");
    textureManager->LoadDDS(device, cq,
        "metalSAO",
        L"Assets\\Metal_Semirough_01_ambientocclusion.dds");
    textureManager->LoadDDS(device, cq,
        "metalSMetalness",
        L"Assets\\Metal_Semirough_01_metallic.dds");
    textureManager->LoadDDS(device, cq,
        "metalSRoughness",
        L"Assets\\Metal_Semirough_01_roughness.dds");

    textureManager->LoadDDS(device, cq,
        "ornamentAlbedo",
        L"Assets\\Metal_Ornament_01_basecolor.dds");
    textureManager->LoadDDS(device, cq,
        "ornamentNormal",
        L"Assets\\Metal_Ornament_01_normal.dds");
    textureManager->LoadDDS(device, cq,
        "ornamentAO",
        L"Assets\\Metal_Ornament_01_ambientocclusion.dds");
    textureManager->LoadDDS(device, cq,
        "ornamentMetalness",
        L"Assets\\Metal_Ornament_01_metallic.dds");
    textureManager->LoadDDS(device, cq,
        "ornamentRoughness",
        L"Assets\\Metal_Ornament_01_roughness.dds");

    textureManager->LoadDDS(device, cq,
        "bark_albedo",
        L"Assets\\Tree_Bark_sb0jlop0_1K_BaseColor.dds");
    textureManager->LoadDDS(device, cq,
        "bark_normal",
        L"Assets\\Tree_Bark_sb0jlop0_1K_Normal.dds");
    textureManager->LoadDDS(device, cq,
        "bark_roughness",
        L"Assets\\Tree_Bark_sb0jlop0_1K_Roughness.dds");
    textureManager->LoadDDS(device, cq,
        "bark_ao",
        L"Assets\\Tree_Bark_sb0jlop0_1K_AO.dds");

    textureManager->LoadDDS(device, cq,
        "bark2_albedo",
        L"Assets\\Tree_Bark_vimmdcofw_2K_BaseColor.dds");
    textureManager->LoadDDS(device, cq,
        "bark2_normal",
        L"Assets\\Tree_Bark_vimmdcofw_2K_Normal.dds");
    textureManager->LoadDDS(device, cq,
        "bark2_roughness",
        L"Assets\\Tree_Bark_vimmdcofw_2K_Roughness.dds");
    textureManager->LoadDDS(device, cq,
        "bark2_ao",
        L"Assets\\Tree_Bark_vimmdcofw_2K_AO.dds");

    textureManager->LoadDDS(device, cq,
        "bark3_albedo",
        L"Assets\\Pine_Bark_vmbibe2g_2K_BaseColor.dds");
    textureManager->LoadDDS(device, cq,
        "bark3_normal",
        L"Assets\\Pine_Bark_vmbibe2g_2K_Normal.dds");
    textureManager->LoadDDS(device, cq,
        "bark3_roughness",
        L"Assets\\Pine_Bark_vmbibe2g_2K_Roughness.dds");
    textureManager->LoadDDS(device, cq,
        "bark3_ao",
        L"Assets\\Pine_Bark_vmbibe2g_2K_AO.dds");

    textureManager->LoadDDS(device, cq,
        "leaf_albedo",
        L"Assets\\Birch_qghn02_1K_BaseColor.dds"
    );
    textureManager->LoadDDS(device, cq,
        "leaf_normal",
        L"Assets\\Birch_qghn02_1K_Normal.dds"
    );
    textureManager->LoadDDS(device, cq,
        "leaf_ao",
        L"Assets\\Birch_qghn02_1K_AO.dds"
    );
    textureManager->LoadDDS(device, cq,
        "leaf_roughness",
        L"Assets\\Birch_qghn02_1K_Roughness.dds"
    );

    textureManager->LoadDDS(device, cq,
        "bay_leaf_albedo",
        L"Assets\\Bay_Leaf_oh0eedvh2_1K_BaseColor.dds"
    );
    textureManager->LoadDDS(device, cq,
        "bay_leaf_normal",
        L"Assets\\Bay_Leaf_oh0eedvh2_1K_Normal.dds"
    );
    textureManager->LoadDDS(device, cq,
        "bay_leaf_ao",
        L"Assets\\Bay_Leaf_oh0eedvh2_1K_AO.dds"
    );
    textureManager->LoadDDS(device, cq,
        "bay_leaf_roughness",
        L"Assets\\Bay_Leaf_oh0eedvh2_1K_Roughness.dds"
    );

    textureManager->LoadDDS(device, cq,
        "forest_floor_albedo",
        L"Assets\\Forest_Floor_vktfeilaw_1K_BaseColor.dds");
    textureManager->LoadDDS(device, cq,
        "forest_floor_normal",
        L"Assets\\Forest_Floor_vktfeilaw_1K_Normal.dds");
    textureManager->LoadDDS(device, cq,
        "forest_floor_roughness",
        L"Assets\\Forest_Floor_vktfeilaw_1K_Roughness.dds");
    textureManager->LoadDDS(device, cq,
        "forest_floor_ao",
        L"Assets\\Forest_Floor_vktfeilaw_1K_AO.dds");
#pragma endregion

    // TODO: Don't create the physics objects here, they shouldn't be recreated if the device is lost
    Scene::CreateScene(device, cq);
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    using namespace Gradient;

    auto device = m_deviceResources->GetD3DDevice();
    auto cq = m_deviceResources->GetCommandQueue();
    auto cl = m_deviceResources->GetCommandList();
    Gradient::GraphicsMemoryManager::Initialize(device);
    Rendering::TextureDrawer::CreateRootSignature(device);

    // Initialize ImGUI

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device;
    initInfo.CommandQueue = cq;
    initInfo.NumFramesInFlight = 2;
    initInfo.RTVFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

    auto gmm = Gradient::GraphicsMemoryManager::Get();
    initInfo.SrvDescriptorHeap = gmm->GetSrvDescriptorHeap();
    initInfo.SrvDescriptorAllocFn
        = [](ImGui_ImplDX12_InitInfo*,
            D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
        {
            auto gmm = Gradient::GraphicsMemoryManager::Get();
            auto index = gmm->AllocateSrv();
            *outCpuHandle = gmm->GetSRVCpuHandle(index);
            *outGpuHandle = gmm->GetSRVGpuHandle(index);
        };
    initInfo.SrvDescriptorFreeFn
        = [](ImGui_ImplDX12_InitInfo*,
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
        {
            auto gmm = Gradient::GraphicsMemoryManager::Get();
            gmm->FreeSrvByCpuHandle(cpuHandle);
        };


    ImGui_ImplDX12_Init(&initInfo);

    EntityManager::Initialize();
    TextureManager::Initialize(device, cq);

    m_renderer->CreateWindowSizeIndependentResources(device, cq);

    // TODO: Don't duplicate this
    auto waterParams = Params::Water{
        50.f, 400.f
    };
    m_renderingWindow.Water = waterParams;

    CreateEntities();

    // TODO: Move this logic into the rendering window
    auto params = m_renderer->PointLightParams();
    for (int i = 0; i < params.size(); i++)
    {
        m_renderingWindow.PointLights[i]
            = params[i];
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto windowSize = m_deviceResources->GetOutputSize();
    m_camera.SetAspectRatio((float)windowSize.right / (float)windowSize.bottom);
    m_character->SetAspectRatio((float)windowSize.right / (float)windowSize.bottom);

    auto device = m_deviceResources->GetD3DDevice();
    auto cl = m_deviceResources->GetCommandList();
    auto cq = m_deviceResources->GetCommandQueue();
    auto width = static_cast<UINT>(windowSize.right);
    auto height = static_cast<UINT>(windowSize.bottom);

    m_tonemappedRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
        device,
        width,
        height,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        false
    );

    m_renderer->CreateWindowSizeDependentResources(device,
        cq,
        windowSize);

    m_renderingWindow.LightColour = m_renderer->DirectionalLight->GetColour().ToVector3();
    m_renderingWindow.LightDirection = m_renderer->DirectionalLight->GetDirection();
    m_renderingWindow.Irradiance = m_renderer->DirectionalLight->GetIrradiance();
    m_renderingWindow.AmbientIrradiance = 1.f;
    m_renderingWindow.BloomExposure = m_renderer->BloomProcessor->GetExposure();
    m_renderingWindow.BloomIntensity = m_renderer->BloomProcessor->GetIntensity();
}

void Game::OnDeviceLost()
{
    auto entityManager = Gradient::EntityManager::Get();
    entityManager->OnDeviceLost();

    Gradient::Rendering::TextureDrawer::Shutdown();
    Gradient::GraphicsMemoryManager::Shutdown();
    // TODO: Fill this out maybe
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
