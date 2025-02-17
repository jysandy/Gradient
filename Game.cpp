//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "directxtk12/Keyboard.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/TextureManager.h"
#include "Core/Rendering/TextureDrawer.h"
#include "Core/Rendering/GeometricPrimitive.h"
#include "Core/Parameters.h"
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

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    Gradient::Physics::PhysicsEngine::Initialize();
    m_deviceResources->SetWindow(window, width, height);

    m_renderer = std::make_unique<Gradient::Rendering::Renderer>();

    m_deviceResources->CreateDeviceResources();
    //m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT,
    //    D3D12_RESOURCE_STATE_COPY_DEST);

    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
    //m_deviceResources->Present(D3D12_RESOURCE_STATE_COPY_DEST);

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
    m_mouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
    m_camera.SetPosition(Vector3{ 0, 30, 25 });

    auto cq = m_deviceResources->GetCommandQueue();

    Gradient::Physics::PhysicsEngine::Get()->StartSimulation();
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
    m_camera.Update(timer);

    auto entityManager = Gradient::EntityManager::Get();

    entityManager->UpdateAll(timer);

    m_physicsWindow.Update();
    if (Gradient::Physics::PhysicsEngine::Get()->IsPaused())
    {
        m_entityWindow.Enable();
    }
    else
    {
        m_entityWindow.Disable();
    }

    m_entityWindow.Update();
    m_perfWindow.FPS = timer.GetFramesPerSecond();

    m_renderer->DirectionalLight->SetLightDirection(m_renderingWindow.LightDirection);
    m_renderer->DirectionalLight->SetColour(m_renderingWindow.GetLinearLightColour());
    m_renderer->DirectionalLight->SetIrradiance(m_renderingWindow.Irradiance);

    for (int i = 0; i < m_renderer->PointLights.size(); i++)
    {
        m_renderer->PointLights[i].SetParams(m_renderingWindow.PointLights[i]);
    }

    m_renderer->SkyDomePipeline->SetAmbientIrradiance(m_renderingWindow.AmbientIrradiance);
    auto totalSeconds = m_timer.GetTotalSeconds();

    m_renderer->WaterPipeline->SetTotalTime(totalSeconds);
    m_renderer->WaterPipeline->SetWaterParams(m_renderingWindow.Water);
    m_renderer->BloomProcessor->SetExposure(m_renderingWindow.BloomExposure);
    m_renderer->BloomProcessor->SetIntensity(m_renderingWindow.BloomIntensity);
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

    m_renderer->Render(cl,
        m_deviceResources->GetScreenViewport(),
        &m_camera,
        m_tonemappedRenderTexture.get());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_tonemappedRenderTexture->SetAsTarget(cl);
    m_physicsWindow.Draw();
    m_entityWindow.Draw();
    m_renderingWindow.Draw();
    m_perfWindow.Draw();

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
#pragma endregion

    JPH::BodyInterface& bodyInterface
        = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

    // TODO: Don't create the physics objects here, they shouldn't be recreated if the device is lost

    Entity sphere1;
    sphere1.id = "sphere1";
    sphere1.Drawable = Rendering::GeometricPrimitive::CreateSphere(device,
        cq, 2.f);
    sphere1.RenderPipeline = m_renderer->PbrPipeline.get();
    sphere1.Material = Rendering::PBRMaterial(
        "metalSAlbedo",
        "metalSNormal",
        "metalSAO",
        "metalSMetalness",
        "metalSRoughness"
    );
    JPH::BodyCreationSettings sphere1Settings(
        new JPH::SphereShape(1.f),
        JPH::RVec3(-3.f, 3.f + 10.f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::ObjectLayers::MOVING
    );

    sphere1Settings.mRestitution = 0.9f;
    sphere1Settings.mLinearVelocity = JPH::Vec3{ 0, 1.5f, 0 };
    sphere1.BodyID = bodyInterface.CreateAndAddBody(sphere1Settings, JPH::EActivation::Activate);
    entityManager->AddEntity(std::move(sphere1));

    Entity sphere2;
    sphere2.id = "sphere2";
    sphere2.Drawable = Rendering::GeometricPrimitive::CreateSphere(device,
        cq, 2.f);
    sphere2.RenderPipeline = m_renderer->PbrPipeline.get();
    sphere2.Material = Rendering::PBRMaterial(
        "ornamentAlbedo",
        "ornamentNormal",
        "ornamentAO",
        "ornamentMetalness",
        "ornamentRoughness"
    );
    sphere2.Translation = Matrix::CreateTranslation(Vector3{ 3.f, 5.f, 0.f });
    JPH::BodyCreationSettings sphere2Settings(
        new JPH::SphereShape(1.f),
        JPH::RVec3(3.f, 5.f + 10.f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::ObjectLayers::MOVING
    );
    sphere2Settings.mRestitution = 0.9f;
    sphere2.BodyID = bodyInterface.CreateAndAddBody(sphere2Settings, JPH::EActivation::Activate);
    entityManager->AddEntity(std::move(sphere2));

    Entity floor;
    floor.id = "floor";
    floor.Drawable = Rendering::GeometricPrimitive::CreateBox(device,
        cq, Vector3{ 20.f, 0.5f, 20.f });
    floor.RenderPipeline = m_renderer->PbrPipeline.get();
    floor.Material = Rendering::PBRMaterial(
        "tiles06Albedo",
        "tiles06Normal",
        "tiles06AO",
        "tiles06Metalness",
        "tiles06Roughness"
    );
    floor.Translation = Matrix::CreateTranslation(Vector3{ 0.f, -0.25f + 10.f, 0.f });

    JPH::BoxShape* floorShape = new JPH::BoxShape(JPH::Vec3(10.f, 0.25f, 10.f));
    JPH::BodyCreationSettings floorSettings(
        floorShape,
        JPH::RVec3(0.f, -0.25f + 10.f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Gradient::Physics::ObjectLayers::NON_MOVING
    );

    auto floorBodyId = bodyInterface.CreateAndAddBody(floorSettings, JPH::EActivation::DontActivate);
    floor.BodyID = floorBodyId;
    entityManager->AddEntity(std::move(floor));

    Entity box1;
    box1.id = "box1";
    box1.Drawable = Rendering::GeometricPrimitive::CreateBox(device,
        cq, Vector3{ 3.f, 3.f, 3.f });
    box1.RenderPipeline = m_renderer->PbrPipeline.get();
    box1.Material = Rendering::PBRMaterial(
        "metal01Albedo",
        "metal01Normal",
        "metal01AO",
        "metal01Roughness",
        "metal01Metalness"
    );
    box1.Translation = Matrix::CreateTranslation(Vector3{ -5.f, 1.5f, -4.f });
    JPH::BoxShape* box1Shape = new JPH::BoxShape(JPH::Vec3(1.5f, 1.5f, 1.5f));
    JPH::BodyCreationSettings box1Settings(
        box1Shape,
        JPH::RVec3(-5.f, 1.5f + 10.f, -4.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Gradient::Physics::ObjectLayers::MOVING
    );
    auto box1BodyId = bodyInterface.CreateAndAddBody(box1Settings, JPH::EActivation::Activate);
    box1.BodyID = box1BodyId;
    entityManager->AddEntity(std::move(box1));

    Entity box2;
    box2.id = "box2";
    box2.Drawable = Rendering::GeometricPrimitive::CreateBox(device,
        cq, Vector3{ 3.f, 3.f, 3.f });
    box2.RenderPipeline = m_renderer->PbrPipeline.get();
    box2.Material = Rendering::PBRMaterial::DefaultPBRMaterial();
    box2.Material.Texture = textureManager->GetTexture("crate");
    box2.Material.NormalMap = textureManager->GetTexture("crateNormal");
    box2.Material.AOMap = textureManager->GetTexture("crateAO");
    box2.Material.RoughnessMap = textureManager->GetTexture("crateRoughness");
    box2.Translation = Matrix::CreateTranslation(Vector3{ -5.f, 1.5f, -4.f });
    JPH::BoxShape* box2Shape = new JPH::BoxShape(JPH::Vec3(1.5f, 1.5f, 1.5f));
    JPH::BodyCreationSettings box2Settings(
        box2Shape,
        JPH::RVec3(-5.f, 1.5f + 10.f, 1.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Gradient::Physics::ObjectLayers::MOVING
    );
    auto box2BodyId = bodyInterface.CreateAndAddBody(box2Settings, JPH::EActivation::Activate);
    box2.BodyID = box2BodyId;
    entityManager->AddEntity(std::move(box2));

    Entity water;
    water.id = "water";
    water.Drawable = Rendering::GeometricPrimitive::CreateGrid(device,
        cq,
        800,
        800,
        100);
    water.RenderPipeline = m_renderer->WaterPipeline.get();
    water.CastsShadows = false;
    entityManager->AddEntity(std::move(water));

    Entity ePointLight1;
    ePointLight1.id = "pointLight1";
    ePointLight1.Drawable = Rendering::GeometricPrimitive::CreateSphere(device,
        cq,
        0.5f);
    ePointLight1.RenderPipeline = m_renderer->PbrPipeline.get();
    // Black
    ePointLight1.Material = Rendering::PBRMaterial::DefaultPBRMaterial();
    ePointLight1.Material.Texture = textureManager->GetTexture("defaultMetalness");
    ePointLight1.CastsShadows = true;
    ePointLight1.Material.EmissiveRadiance = 7 * Vector3{ 0.9, 0.8, 0.5 };
    Rendering::PointLight pointLight1;
    pointLight1.EntityId = ePointLight1.id;
    pointLight1.Colour = Color(Vector3{ 0.9, 0.8, 0.5 });
    pointLight1.Irradiance = 7.f;
    pointLight1.MaxRange = 10.f;
    pointLight1.ShadowCubeIndex = 0;
    // TODO: Extract these point lights into components
    m_renderer->PointLights.push_back(pointLight1);
    JPH::BodyCreationSettings ePointLight1Settings(
        new JPH::SphereShape(0.25f),
        JPH::RVec3(-5.f, 20.f, 5.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::ObjectLayers::MOVING
    );
    ePointLight1Settings.mRestitution = 0.9f;
    ePointLight1.BodyID
        = bodyInterface.CreateAndAddBody(ePointLight1Settings,
            JPH::EActivation::Activate);
    entityManager->AddEntity(std::move(ePointLight1));

    Entity ePointLight2;
    ePointLight2.id = "pointLight2";
    ePointLight2.Drawable = Rendering::GeometricPrimitive::CreateSphere(device,
        cq,
        0.5f);
    ePointLight2.RenderPipeline = m_renderer->PbrPipeline.get();
    ePointLight2.CastsShadows = true;
    ePointLight2.Material = Rendering::PBRMaterial::DefaultPBRMaterial();
    ePointLight2.Material.EmissiveRadiance = 7 * Vector3{ 1, 0.3, 0 };
    // Black
    ePointLight2.Material.Texture = textureManager->GetTexture("defaultMetalness");
    Rendering::PointLight pointLight2;
    pointLight2.EntityId = ePointLight2.id;
    pointLight2.Colour = Color(Vector3{ 1, 0.3, 0 });
    pointLight2.Irradiance = 7.f;
    pointLight2.MaxRange = 10.f;
    pointLight2.ShadowCubeIndex = 1;
    m_renderer->PointLights.push_back(pointLight2);
    JPH::BodyCreationSettings ePointLight2Settings(
        new JPH::SphereShape(0.25f),
        JPH::RVec3(8.f, 20, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::ObjectLayers::MOVING
    );
    ePointLight2Settings.mRestitution = 0.9f;
    ePointLight2.BodyID
        = bodyInterface.CreateAndAddBody(ePointLight2Settings,
            JPH::EActivation::Activate);
    entityManager->AddEntity(std::move(ePointLight2));

    Entity terrain;
    terrain.id = "terrain";
    terrain.Drawable = Rendering::GeometricPrimitive::CreateGrid(device,
        cq,
        100,
        100,
        10,
        false);
    terrain.RenderPipeline = m_renderer->HeightmapPipeline.get();
    terrain.CastsShadows = true;
    terrain.SetTranslation(Vector3{ 50, -1, 0 });
    entityManager->AddEntity(std::move(terrain));
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

    for (int i = 0; i < m_renderer->PointLights.size(); i++)
    {
        m_renderingWindow.PointLights[i]
            = m_renderer->PointLights[i].AsParams();
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto windowSize = m_deviceResources->GetOutputSize();
    m_camera.SetAspectRatio((float)windowSize.right / (float)windowSize.bottom);

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
