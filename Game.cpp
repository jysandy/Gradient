//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "directxtk/Keyboard.h"
#include "Core/TextureManager.h"
#include "Core/Rendering/GeometricPrimitive.h"
#include "Core/Parameters.h"
#include <directxtk/SimpleMath.h>

#include <imgui.h>
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx11.h"
#include "Core/ReadData.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_D32_FLOAT,
        2,
        D3D_FEATURE_LEVEL_11_1,
        DX::DeviceResources::c_FlipPresent | DX::DeviceResources::c_AllowTearing
    );
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    Gradient::Physics::PhysicsEngine::Initialize();
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
    m_mouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
    m_camera.SetPosition(Vector3{ 0, 30, 25 });

    // Initialize ImGUI

    ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());
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

    m_dLight->SetLightDirection(m_renderingWindow.LightDirection);
    m_dLight->SetColour(m_renderingWindow.GetLinearLightColour());
    m_dLight->SetIrradiance(m_renderingWindow.Irradiance);

    for (int i = 0; i < m_pointLights.size(); i++)
    {
        m_pointLights[i].SetParams(m_renderingWindow.PointLights[i]);
    }

    m_skyDomePipeline->SetAmbientIrradiance(m_renderingWindow.AmbientIrradiance);
    auto totalSeconds = m_timer.GetTotalSeconds();

    m_waterPipeline->SetTotalTime(totalSeconds);
    m_waterPipeline->SetWaterParams(m_renderingWindow.Water);
    m_waterShadowPipeline->SetTotalTime(totalSeconds);
    m_waterShadowPipeline->SetWaterParams(m_renderingWindow.Water);
    m_bloomProcessor->SetExposure(m_renderingWindow.BloomExposure);
    m_bloomProcessor->SetIntensity(m_renderingWindow.BloomIntensity);
}
#pragma endregion

std::vector<Gradient::Params::PointLight> Game::PointLightParams()
{
    std::vector<Gradient::Params::PointLight> out;
    for (const auto& light : m_pointLights)
    {
        out.push_back(light.AsParams());
    }

    return out;
}

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    auto entityManager = Gradient::EntityManager::Get();
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_dLight->ClearAndSetDSV(context);

    m_deviceResources->PIXBeginEvent(L"Shadow Pass");

    // Camera position is needed for tessellation. 
    // But direction is not needed because culling is 
    // disabled when drawing water shadows.
    m_waterShadowPipeline->SetCameraPosition(m_camera.GetPosition());

    m_shadowMapPipeline->SetView(m_dLight->GetView());
    m_waterShadowPipeline->SetView(m_dLight->GetView());
    m_shadowMapPipeline->SetProjection(m_dLight->GetProjection());
    m_waterShadowPipeline->SetProjection(m_dLight->GetProjection());

    entityManager->DrawAll(context, true);

    for (auto& pointLight : m_pointLights)
    {
        m_shadowCubeArray->Render(context,
            pointLight.ShadowCubeIndex,
            pointLight.AsParams().Position,
            pointLight.MinRange,
            pointLight.MaxRange,
            [=](SimpleMath::Matrix view, SimpleMath::Matrix proj)
            {
                m_shadowMapPipeline->SetView(view);
                m_waterShadowPipeline->SetView(view);
                m_shadowMapPipeline->SetProjection(proj);
                m_waterShadowPipeline->SetProjection(proj);
                entityManager->DrawAll(context, true);
            });
    }

    m_deviceResources->PIXEndEvent();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_skyDomePipeline->SetDirectionalLight(m_dLight.get());

    m_deviceResources->PIXBeginEvent(L"Environment Map");

    m_environmentMap->Render(context,
        [=](SimpleMath::Matrix view, SimpleMath::Matrix proj)
        {
            m_skyDomePipeline->SetSunCircleEnabled(false);
            m_skyDomePipeline->SetProjection(proj);
            m_skyDomePipeline->SetView(view);
            m_skyDomePipeline->Apply(context);
            m_sky->Draw(context);
        });

    m_deviceResources->PIXEndEvent();

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    m_skyDomePipeline->SetSunCircleEnabled(true);
    m_skyDomePipeline->SetProjection(m_camera.GetProjectionMatrix());
    m_skyDomePipeline->SetView(m_camera.GetViewMatrix());
    m_skyDomePipeline->Apply(context);
    m_sky->Draw(context);

    m_pbrPipeline->SetCameraPosition(m_camera.GetPosition());
    m_pbrPipeline->SetDirectionalLight(m_dLight.get());
    m_pbrPipeline->SetView(m_camera.GetViewMatrix());
    m_pbrPipeline->SetProjection(m_camera.GetProjectionMatrix());
    m_pbrPipeline->SetEnvironmentMap(m_environmentMap->GetSRV());
    m_pbrPipeline->SetPointLights(PointLightParams());
    m_pbrPipeline->SetShadowCubeArray(m_shadowCubeArray->GetSRV());

    m_waterPipeline->SetCameraPosition(m_camera.GetPosition());
    m_waterPipeline->SetCameraDirection(m_camera.GetDirection());
    m_waterPipeline->SetDirectionalLight(m_dLight.get());
    m_waterPipeline->SetView(m_camera.GetViewMatrix());
    m_waterPipeline->SetProjection(m_camera.GetProjectionMatrix());
    m_waterPipeline->SetEnvironmentMap(m_environmentMap->GetSRV());
    m_waterPipeline->SetPointLights(PointLightParams());
    m_waterPipeline->SetShadowCubeArray(m_shadowCubeArray->GetSRV());

    entityManager->DrawAll(context);

    m_multisampledRenderTexture->CopyToSingleSampled(context);
    m_deviceResources->PIXEndEvent();

    // Post-process ----
    m_deviceResources->PIXBeginEvent(L"Bloom");
    auto bloomOutput = m_bloomProcessor->Process(context,
        m_multisampledRenderTexture.get());
    m_deviceResources->PIXEndEvent();

    // Tonemap and draw GUI
    bloomOutput->DrawTo(context,
        m_tonemappedRenderTexture.get(),
        [=]
        {
            context->PSSetShader(m_tonemapperPS.Get(), nullptr, 0);
        });

    m_tonemappedRenderTexture->SetAsTarget(context);
    m_physicsWindow.Draw();
    m_entityWindow.Draw();
    m_renderingWindow.Draw();
    m_perfWindow.Draw();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    context->CopyResource(m_deviceResources->GetRenderTarget(),
        m_tonemappedRenderTexture->GetSingleSampledTexture());

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_multisampledRenderTexture->ClearAndSetAsTarget(context);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
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

void Game::OnQuit()
{
    Gradient::Physics::PhysicsEngine::Shutdown();
    Gradient::TextureManager::Shutdown();
    Gradient::EntityManager::Shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
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
    auto context = m_deviceResources->GetD3DDeviceContext();

#pragma region Textures
    // TODO: display a loading screen while loading all these textures
    // TODO: cut down on the texture size, these are way too big

    textureManager->LoadDDS(device, context,
        "crate",
        L"Assets\\Wood_Crate_001_basecolor.dds");
    textureManager->LoadDDS(device, context,
        "crateNormal",
        L"Assets\\Wood_Crate_001_normal.dds");
    textureManager->LoadDDS(device, context,
        "crateRoughness",
        L"Assets\\Wood_Crate_001_roughness.dds");
    textureManager->LoadDDS(device, context,
        "crateAO",
        L"Assets\\Wood_Crate_001_ambientOcclusion.dds");

    textureManager->LoadDDS(device, context,
        "tilesAlbedo",
        L"Assets\\TilesDiffuse.dds");
    textureManager->LoadDDS(device, context,
        "tilesNormal",
        L"Assets\\TilesNormal.dds");
    textureManager->LoadDDS(device, context,
        "tilesAO",
        L"Assets\\TilesAO.dds");
    textureManager->LoadDDS(device, context,
        "tilesMetalness",
        L"Assets\\TilesMetalness.dds");
    textureManager->LoadDDS(device, context,
        "tilesRoughness",
        L"Assets\\TilesRoughness.dds");

    textureManager->LoadDDS(device, context,
        "tiles06Albedo",
        L"Assets\\Tiles_Decorative_06_basecolor.dds");
    textureManager->LoadDDS(device, context,
        "tiles06Normal",
        L"Assets\\Tiles_Decorative_06_normal.dds");
    textureManager->LoadDDS(device, context,
        "tiles06AO",
        L"Assets\\Tiles_Decorative_06_ambientocclusion.dds");
    textureManager->LoadDDS(device, context,
        "tiles06Metalness",
        L"Assets\\Tiles_Decorative_06_metallic.dds");
    textureManager->LoadDDS(device, context,
        "tiles06Roughness",
        L"Assets\\Tiles_Decorative_06_roughness.dds");

    textureManager->LoadDDS(device, context,
        "metal01Albedo",
        L"Assets\\Metal_Floor_01_basecolor.dds");
    textureManager->LoadDDS(device, context,
        "metal01Normal",
        L"Assets\\Metal_Floor_01_normal.dds");
    textureManager->LoadDDS(device, context,
        "metal01AO",
        L"Assets\\Metal_Floor_01_ambientocclusion.dds");
    textureManager->LoadDDS(device, context,
        "metal01Metalness",
        L"Assets\\Metal_Floor_01_metallic.dds");
    textureManager->LoadDDS(device, context,
        "metal01Roughness",
        L"Assets\\Metal_Floor_01_roughness.dds");

    textureManager->LoadDDS(device, context,
        "metalSAlbedo",
        L"Assets\\Metal_Semirough_01_basecolor.dds");
    textureManager->LoadDDS(device, context,
        "metalSNormal",
        L"Assets\\Metal_Semirough_01_normal.dds");
    textureManager->LoadDDS(device, context,
        "metalSAO",
        L"Assets\\Metal_Semirough_01_ambientocclusion.dds");
    textureManager->LoadDDS(device, context,
        "metalSMetalness",
        L"Assets\\Metal_Semirough_01_metallic.dds");
    textureManager->LoadDDS(device, context,
        "metalSRoughness",
        L"Assets\\Metal_Semirough_01_roughness.dds");

    textureManager->LoadDDS(device, context,
        "ornamentAlbedo",
        L"Assets\\Metal_Ornament_01_basecolor.dds");
    textureManager->LoadDDS(device, context,
        "ornamentNormal",
        L"Assets\\Metal_Ornament_01_normal.dds");
    textureManager->LoadDDS(device, context,
        "ornamentAO",
        L"Assets\\Metal_Ornament_01_ambientocclusion.dds");
    textureManager->LoadDDS(device, context,
        "ornamentMetalness",
        L"Assets\\Metal_Ornament_01_metallic.dds");
    textureManager->LoadDDS(device, context,
        "ornamentRoughness",
        L"Assets\\Metal_Ornament_01_roughness.dds");
#pragma endregion

    auto deviceContext = m_deviceResources->GetD3DDeviceContext();
    JPH::BodyInterface& bodyInterface
        = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();

    // TODO: Don't create the physics objects here, they shouldn't be recreated if the device is lost

    Entity sphere1;
    sphere1.id = "sphere1";
    sphere1.Drawable = Rendering::GeometricPrimitive::CreateSphere(device, deviceContext, 2.f);
    sphere1.RenderPipeline = m_pbrPipeline.get();
    sphere1.ShadowPipeline = m_shadowMapPipeline.get();
    sphere1.Texture = textureManager->GetTexture("metalSAlbedo");
    sphere1.NormalMap = textureManager->GetTexture("metalSNormal");
    sphere1.AOMap = textureManager->GetTexture("metalSAO");
    sphere1.MetalnessMap = textureManager->GetTexture("metalSMetalness");
    sphere1.RoughnessMap = textureManager->GetTexture("metalSRoughness");
    sphere1.Translation = Matrix::CreateTranslation(Vector3{ -3.f, 3.f, 0.f });
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
    sphere2.Drawable = Rendering::GeometricPrimitive::CreateSphere(device, deviceContext, 2.f);
    sphere2.RenderPipeline = m_pbrPipeline.get();
    sphere2.ShadowPipeline = m_shadowMapPipeline.get();
    sphere2.Texture = textureManager->GetTexture("ornamentAlbedo");
    sphere2.NormalMap = textureManager->GetTexture("ornamentNormal");
    sphere2.AOMap = textureManager->GetTexture("ornamentAO");
    sphere2.MetalnessMap = textureManager->GetTexture("ornamentMetalness");
    sphere2.RoughnessMap = textureManager->GetTexture("ornamentRoughness");
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
    floor.Drawable = Rendering::GeometricPrimitive::CreateBox(device, deviceContext, Vector3{ 20.f, 0.5f, 20.f });
    floor.RenderPipeline = m_pbrPipeline.get();
    floor.ShadowPipeline = m_shadowMapPipeline.get();
    floor.Texture = textureManager->GetTexture("tiles06Albedo");
    floor.NormalMap = textureManager->GetTexture("tiles06Normal");
    floor.AOMap = textureManager->GetTexture("tiles06AO");
    floor.MetalnessMap = textureManager->GetTexture("tiles06Metalness");
    floor.RoughnessMap = textureManager->GetTexture("tiles06Roughness");
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
    box1.Drawable = Rendering::GeometricPrimitive::CreateBox(device, deviceContext, Vector3{ 3.f, 3.f, 3.f });
    box1.RenderPipeline = m_pbrPipeline.get();
    box1.ShadowPipeline = m_shadowMapPipeline.get();
    box1.Texture = textureManager->GetTexture("metal01Albedo");
    box1.NormalMap = textureManager->GetTexture("metal01Normal");
    box1.AOMap = textureManager->GetTexture("metal01AO");
    box1.RoughnessMap = textureManager->GetTexture("metal01Roughness");
    box1.MetalnessMap = textureManager->GetTexture("metal01Metalness");
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
    box2.Drawable = Rendering::GeometricPrimitive::CreateBox(device, deviceContext, Vector3{ 3.f, 3.f, 3.f });
    box2.RenderPipeline = m_pbrPipeline.get();
    box2.ShadowPipeline = m_shadowMapPipeline.get();
    box2.Texture = textureManager->GetTexture("crate");
    box2.NormalMap = textureManager->GetTexture("crateNormal");
    box2.AOMap = textureManager->GetTexture("crateAO");
    box2.RoughnessMap = textureManager->GetTexture("crateRoughness");
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
        deviceContext,
        800,
        800,
        100);
    water.RenderPipeline = m_waterPipeline.get();
    water.ShadowPipeline = m_waterShadowPipeline.get();
    water.CastsShadows = false;
    entityManager->AddEntity(std::move(water));

    Entity ePointLight1;
    ePointLight1.id = "pointLight1";
    ePointLight1.Drawable = Rendering::GeometricPrimitive::CreateSphere(device,
        deviceContext,
        0.5f);
    ePointLight1.RenderPipeline = m_pbrPipeline.get();
    ePointLight1.ShadowPipeline = m_shadowMapPipeline.get();
    // Black
    ePointLight1.Texture = textureManager->GetTexture("defaultMetalness");
    ePointLight1.CastsShadows = true;
    ePointLight1.EmissiveRadiance = 7 * Vector3{ 0.9, 0.8, 0.5 };
    Rendering::PointLight pointLight1;
    pointLight1.EntityId = ePointLight1.id;
    pointLight1.Colour = Color(Vector3{ 0.9, 0.8, 0.5 });
    pointLight1.Irradiance = 7.f; 
    pointLight1.MaxRange = 10.f;
    pointLight1.ShadowCubeIndex = 0;
    m_pointLights.push_back(pointLight1);
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
        deviceContext,
        0.5f);
    ePointLight2.RenderPipeline = m_pbrPipeline.get();
    ePointLight2.ShadowPipeline = m_shadowMapPipeline.get();
    ePointLight2.CastsShadows = true;
    ePointLight2.EmissiveRadiance = 7 * Vector3{ 1, 0.3, 0 };
    // Black
    ePointLight2.Texture = textureManager->GetTexture("defaultMetalness");
    Rendering::PointLight pointLight2;
    pointLight2.EntityId = ePointLight2.id;
    pointLight2.Colour = Color(Vector3{ 1, 0.3, 0 });
    pointLight2.Irradiance = 7.f;
    pointLight2.MaxRange = 10.f;
    pointLight2.ShadowCubeIndex = 1;
    m_pointLights.push_back(pointLight2);
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
}

Microsoft::WRL::ComPtr<ID3D11PixelShader> Game::LoadPixelShader(const std::wstring& path)
{
    auto device = m_deviceResources->GetD3DDevice();
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;

    auto psData = DX::ReadData(path.c_str());
    DX::ThrowIfFailed(
        device->CreatePixelShader(psData.data(),
            psData.size(),
            nullptr,
            ps.ReleaseAndGetAddressOf()));

    return ps;
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    using namespace Gradient;

    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_states = std::make_shared<DirectX::CommonStates>(device);
    m_pbrPipeline = std::make_unique<Pipelines::PBRPipeline>(device, m_states);
    m_waterPipeline = std::make_unique<Pipelines::WaterPipeline>(device, m_states);
    m_waterShadowPipeline = std::make_unique<Pipelines::WaterShadowPipeline>(device, m_states);
    m_tonemapperPS = LoadPixelShader(L"ACESTonemapper_PS.cso");

    EntityManager::Initialize();
    TextureManager::Initialize(device, context);

    auto dlight = new Gradient::Rendering::DirectionalLight(
        device,
        { -0.7f, -0.3f, 0.7f },
        50.f
    );
    m_dLight = std::unique_ptr<Gradient::Rendering::DirectionalLight>(dlight);
    auto lightColor = DirectX::SimpleMath::Color(0.86, 0.49, 0.06, 1);
    m_dLight->SetColour(lightColor);
    m_dLight->SetIrradiance(7.f);

    m_shadowMapPipeline = std::make_unique<Gradient::Pipelines::ShadowMapPipeline>(device, m_states);
    m_skyDomePipeline = std::make_unique<Gradient::Pipelines::SkyDomePipeline>(device, m_states);
    m_skyDomePipeline->SetAmbientIrradiance(1.f);
    m_sky = Rendering::GeometricPrimitive::CreateGeoSphere(device, context, 2.f, 3,
        false);
    m_environmentMap = std::make_unique<Gradient::Rendering::CubeMap>(device,
        256,
        DXGI_FORMAT_R32G32B32A32_FLOAT);

    auto waterParams = Params::Water{ 
        50.f, 400.f
    };
    m_waterPipeline->SetWaterParams(waterParams);
    m_waterShadowPipeline->SetWaterParams(waterParams);
    m_waterShadowPipeline->SetWaves(m_waterPipeline->GetWaves());
    m_renderingWindow.Water = waterParams;

    CreateEntities();

    m_shadowCubeArray = std::make_unique<Rendering::DepthCubeArray>(device, 
        256, m_pointLights.size());

    for (int i = 0; i < m_pointLights.size(); i++)
    {
        m_renderingWindow.PointLights[i] = m_pointLights[i].AsParams();
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto windowSize = m_deviceResources->GetOutputSize();
    m_camera.SetAspectRatio((float)windowSize.right / (float)windowSize.bottom);

    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto width = static_cast<UINT>(windowSize.right);
    auto height = static_cast<UINT>(windowSize.bottom);

    m_multisampledRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
        device,
        context,
        m_states,
        width,
        height,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        true
    );

    m_tonemappedRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
        device,
        context,
        m_states,
        width,
        height,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        false
    );

    m_bloomProcessor = std::make_unique<Gradient::Rendering::BloomProcessor>(
        device,
        context,
        m_states,
        width,
        height,
        DXGI_FORMAT_R32G32B32A32_FLOAT
    );

    m_bloomProcessor->SetExposure(18.f);
    m_bloomProcessor->SetIntensity(0.3f);

    m_renderingWindow.SetLinearLightColour(m_dLight->GetColour());
    m_renderingWindow.LightDirection = m_dLight->GetDirection();
    m_renderingWindow.Irradiance = m_dLight->GetIrradiance();
    m_renderingWindow.AmbientIrradiance = 1.f;
    m_renderingWindow.BloomExposure = m_bloomProcessor->GetExposure();
    m_renderingWindow.BloomIntensity = m_bloomProcessor->GetIntensity();
}

void Game::OnDeviceLost()
{
    auto entityManager = Gradient::EntityManager::Get();
    entityManager->OnDeviceLost();

    // TODO: Fill this out maybe
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
