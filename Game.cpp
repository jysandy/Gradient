//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "directxtk12/Keyboard.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/TextureManager.h"
#include "Core/Rendering/TextureDrawer.h"
#include "Core/Rendering/ProceduralMesh.h"
#include "Core/Rendering/LSystem.h"
#include "Core/Parameters.h"
#include "Core/ECS/Components/NameTagComponent.h"
#include "Core/ECS/Components/DrawableComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/MaterialComponent.h"
#include "Core/ECS/Components/RigidBodyComponent.h"
#include "Core/ECS/Components/PointLightComponent.h"
#include "Core/ECS/Components/InstanceDataComponent.h"
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
    m_camera.Deactivate();
    m_character->Activate();
    m_physicsWindow.UnpauseSimulation();
}

void Game::StartEditing()
{
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
    m_renderer->DirectionalLight->SetColour(m_renderingWindow.GetLinearLightColour());
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

    auto kb = DirectX::Keyboard::Get().GetState();
    if (kb.OemTilde)
    {
        TogglePlaying(timer.GetTotalSeconds());
    }
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

    m_renderer->Render(cl,
        m_deviceResources->GetScreenViewport(),
        &frameCamera,
        m_tonemappedRenderTexture.get());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_tonemappedRenderTexture->SetAsTarget(cl);

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
#pragma endregion

    JPH::BodyInterface& bodyInterface
        = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

    // TODO: Don't create the physics objects here, they shouldn't be recreated if the device is lost

    auto sphere1 = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(sphere1, "sphere1");
    entityManager->Registry.emplace<TransformComponent>(sphere1);
    entityManager->Registry.emplace<DrawableComponent>(sphere1,
        Rendering::ProceduralMesh::CreateSphere(device,
            cq, 2.f)
    );
    entityManager->Registry.emplace<MaterialComponent>(sphere1,
        Rendering::PBRMaterial(
            "metalSAlbedo",
            "metalSNormal",
            "metalSAO",
            "metalSMetalness",
            "metalSRoughness"
        ));
    entityManager->Registry.emplace<RigidBodyComponent>(sphere1,
        RigidBodyComponent::CreateSphere(2.f,
            Vector3{ -3.f, 3.f + 10.f, 0.f },
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                settings.mLinearVelocity = JPH::Vec3{ 0, 1.5f, 0 };
                return settings;
            }
        ));

    auto sphere2 = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(sphere2, "sphere2");
    entityManager->Registry.emplace<TransformComponent>(sphere2);
    entityManager->Registry.emplace<DrawableComponent>(sphere2,
        Rendering::ProceduralMesh::CreateSphere(device,
            cq, 2.f));
    entityManager->Registry.emplace<MaterialComponent>(sphere2,
        Rendering::PBRMaterial(
            "ornamentAlbedo",
            "ornamentNormal",
            "ornamentAO",
            "ornamentMetalness",
            "ornamentRoughness"
        ));
    entityManager->Registry.emplace<RigidBodyComponent>(sphere2,
        RigidBodyComponent::CreateSphere(
            2.f,
            Vector3{ 3.f, 5.f + 10.f, 0.f },
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                return settings;
            }
        ));

    // Tree

    auto tree = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(tree, "tree");
    auto& frustumTransform
        = entityManager->Registry.emplace<TransformComponent>(tree);
    frustumTransform.Translation = Matrix::CreateTranslation({
        38.6f,
        8.7f,
        0.f });

    Rendering::LSystem lsystem;
    lsystem.AddRule('X', "FFF[/+FX][////+FX]/////////+FX");
    entityManager->Registry.emplace<DrawableComponent>(tree,
        lsystem.Build(device, cq,
            "X", 5));

    entityManager->Registry.emplace<MaterialComponent>(tree,
        Rendering::PBRMaterial(
            "bark_albedo",
            "bark_normal",
            "bark_ao",
            "defaultMetalness",
            "bark_roughness"
        ));

    // End tree

    // Leaves

    auto leaves = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(leaves, "leaves");
    auto& leavesTransform
        = entityManager->Registry.emplace<TransformComponent>(leaves);
    leavesTransform.Translation = Matrix::CreateTranslation({
        20.f,
        20.f,
        10.f
        });
    entityManager->Registry.emplace<MaterialComponent>(leaves,
        Rendering::PBRMaterial(
            "leaf_albedo",
            "leaf_normal",
            "leaf_ao",
            "defaultMetalness",
            "leaf_roughness"
        ));

    // Instance generation

    int numRows = 3;
    int numCols = 4;

    auto& leavesInstance
        = entityManager->Registry.emplace<InstanceDataComponent>(leaves);

    for (int i = 0; i < 1000; i++)
    {
        leavesInstance.Instances.push_back({
                Matrix::CreateTranslation({
                    static_cast<float>(i), 
                    0.f, 
                    0.f}),
                Vector2{0.f, 1.f / numCols},
                Vector2{0.f, 1.f / numRows}
            });
    }

    entityManager->Registry.emplace<DrawableComponent>(leaves,
        Rendering::ProceduralMesh::CreateBillboard(device,
            cq,
            0.5,
            0.5));

    // End leaves


    auto floor = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(floor, "floor");
    auto& floorTransform =
        entityManager->Registry.emplace<TransformComponent>(floor);
    floorTransform.Translation = Matrix::CreateTranslation(Vector3{ 0.f, -0.25f + 10.f, 0.f });
    entityManager->Registry.emplace<DrawableComponent>(floor,
        Rendering::ProceduralMesh::CreateBox(device,
            cq, Vector3{ 20.f, 0.5f, 20.f }));
    entityManager->Registry.emplace<MaterialComponent>(floor,
        Rendering::PBRMaterial(
            "tiles06Albedo",
            "tiles06Normal",
            "tiles06AO",
            "tiles06Metalness",
            "tiles06Roughness"
        ));
    entityManager->Registry.emplace<RigidBodyComponent>(floor,
        RigidBodyComponent::CreateBox(
            Vector3{ 20.f, 0.5f, 20.f },
            Vector3{ 0.f, -0.25f + 10.f, 0.f },
            [](JPH::BodyCreationSettings settings)
            {
                settings.mMotionType = JPH::EMotionType::Static;
                settings.mObjectLayer = Gradient::Physics::ObjectLayers::NON_MOVING;
                return settings;
            }
        ));

    auto box1 = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(box1, "box1");
    entityManager->Registry.emplace<TransformComponent>(box1);
    entityManager->Registry.emplace<DrawableComponent>(box1,
        Rendering::ProceduralMesh::CreateBox(device,
            cq, Vector3{ 3.f, 3.f, 3.f }));
    entityManager->Registry.emplace<MaterialComponent>(box1,
        Rendering::PBRMaterial(
            "metal01Albedo",
            "metal01Normal",
            "metal01AO",
            "metal01Roughness",
            "metal01Metalness"
        ));
    entityManager->Registry.emplace<RigidBodyComponent>(box1,
        RigidBodyComponent::CreateBox(
            Vector3{ 3.f, 3.f, 3.f },
            Vector3{ -5.f, 1.5f + 10.f, -4.f }
        ));

    auto box2 = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(box2, "box2");
    entityManager->Registry.emplace<TransformComponent>(box2);
    entityManager->Registry.emplace<DrawableComponent>(box2, Rendering::ProceduralMesh::CreateBox(device,
        cq, Vector3{ 3.f, 3.f, 3.f }));
    auto& box2MaterialComponent
        = entityManager->Registry.emplace<MaterialComponent>(box2);
    box2MaterialComponent.Material = Rendering::PBRMaterial::DefaultPBRMaterial();
    box2MaterialComponent.Material.Texture = textureManager->GetTexture("crate");
    box2MaterialComponent.Material.NormalMap = textureManager->GetTexture("crateNormal");
    box2MaterialComponent.Material.AOMap = textureManager->GetTexture("crateAO");
    box2MaterialComponent.Material.RoughnessMap = textureManager->GetTexture("crateRoughness");
    entityManager->Registry.emplace<RigidBodyComponent>(box2,
        RigidBodyComponent::CreateBox(
            Vector3{ 3.f, 3.f, 3.f },
            Vector3{ -5.f, 1.5f + 10.f, 1.f }
        ));

    auto water = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(water, "water");
    entityManager->Registry.emplace<TransformComponent>(water);
    entityManager->Registry.emplace<DrawableComponent>(water,
        Rendering::ProceduralMesh::CreateGrid(device,
            cq,
            800,
            800,
            100),
        DrawableComponent::ShadingModel::Water);

    auto ePointLight1 = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(ePointLight1, "pointLight1");
    entityManager->Registry.emplace<TransformComponent>(ePointLight1);
    entityManager->Registry.emplace<DrawableComponent>(ePointLight1,
        Rendering::ProceduralMesh::CreateSphere(device,
            cq,
            0.5f));
    auto& pointLight1MaterialComponent
        = entityManager->Registry.emplace<MaterialComponent>(ePointLight1,
            Rendering::PBRMaterial::DefaultPBRMaterial());
    // Black
    pointLight1MaterialComponent.Material.Texture
        = textureManager->GetTexture("defaultMetalness");
    pointLight1MaterialComponent.Material.EmissiveRadiance
        = 7 * Vector3{ 0.9, 0.8, 0.5 };
    auto& pointLightComponent
        = entityManager->Registry.emplace<PointLightComponent>(ePointLight1);
    pointLightComponent.PointLight.Colour = Color(Vector3{ 0.9, 0.8, 0.5 });
    pointLightComponent.PointLight.Irradiance = 7.f;
    pointLightComponent.PointLight.MaxRange = 10.f;
    pointLightComponent.PointLight.ShadowCubeIndex = 0;
    entityManager->Registry.emplace<RigidBodyComponent>(ePointLight1,
        RigidBodyComponent::CreateSphere(0.5f,
            Vector3{ -5.f, 20.f, 5.f },
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                return settings;
            }));

    auto ePointLight2 = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(ePointLight2, "pointLight2");
    entityManager->Registry.emplace<TransformComponent>(ePointLight2);
    entityManager->Registry.emplace<DrawableComponent>(ePointLight2,
        Rendering::ProceduralMesh::CreateSphere(device,
            cq,
            0.5f));
    auto& pointLight2MaterialComponent
        = entityManager->Registry.emplace<MaterialComponent>(ePointLight2,
            Rendering::PBRMaterial::DefaultPBRMaterial());
    pointLight2MaterialComponent.Material.EmissiveRadiance
        = 7 * Vector3{ 1, 0.3, 0 };
    pointLight2MaterialComponent.Material.Texture
        = textureManager->GetTexture("defaultMetalness");
    auto& pointLight2Component
        = entityManager->Registry.emplace<PointLightComponent>(ePointLight2);
    pointLight2Component.PointLight.Colour = Color(Vector3{ 1, 0.3, 0 });
    pointLight2Component.PointLight.Irradiance = 7.f;
    pointLight2Component.PointLight.MaxRange = 10.f;
    pointLight2Component.PointLight.ShadowCubeIndex = 1;
    entityManager->Registry.emplace<RigidBodyComponent>(ePointLight2,
        RigidBodyComponent::CreateSphere(0.5f,
            Vector3{ 8.f, 20, 0.f },
            [](auto settings)
            {
                settings.mRestitution = 0.9f;
                return settings;
            }));

    textureManager->LoadDDS(device, cq,
        "islandHeightMap",
        L"Assets\\island_height_32bit.dds");

    auto terrain = entityManager->AddEntity();
    entityManager->Registry.emplace<NameTagComponent>(terrain, "terrain");
    auto& terrainTransform = entityManager->Registry.emplace<TransformComponent>(terrain);
    terrainTransform.Translation = Matrix::CreateTranslation(Vector3{ 50, -1, 0 });
    entityManager->Registry.emplace<DrawableComponent>(terrain,
        Rendering::ProceduralMesh::CreateGrid(device,
            cq,
            256,
            256,
            25,
            false),
        DrawableComponent::ShadingModel::Heightmap,
        false);
    entityManager->Registry.emplace<HeightMapComponent>(terrain,
        textureManager->GetTexture("islandHeightMap"),
        10.f,
        256.f);
    entityManager->Registry.emplace<RigidBodyComponent>(terrain,
        RigidBodyComponent::CreateHeightField(L"Assets\\island_height_32bit.dds",
            256.f,
            10.f,
            Vector3{ 50, -1, 0 }));
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

    m_renderingWindow.SetLinearLightColour(m_renderer->DirectionalLight->GetColour());
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
