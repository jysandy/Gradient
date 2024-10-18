//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "directxtk/Keyboard.h"
#include "Core/TextureManager.h"
#include <directxtk/SimpleMath.h>

#include <imgui.h>
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx11.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT,
        2,
        D3D_FEATURE_LEVEL_11_1
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
    m_camera.SetPosition(Vector3{ 0, 0, 25 });

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
}
#pragma endregion

Matrix Game::GetShadowTransform()
{
    const auto t = Matrix(
        0.5f, 0.f, 0.f, 0.f,
        0.f, -0.5f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.5f, 0.5f, 0.f, 1.f
    );

    return m_shadowMapView * m_shadowMapProj * t;
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

    SetShadowMapPipelineState();

    m_deviceResources->PIXBeginEvent(L"Shadow Pass");

    m_shadowMapEffect->SetView(m_shadowMapView);
    m_shadowMapEffect->SetProjection(m_shadowMapProj);
    entityManager->DrawAll(m_shadowMapEffect.get(), [this]()
        {
            auto context = m_deviceResources->GetD3DDeviceContext();
            context->RSSetState(m_shadowMapRSState.Get());
        });

    m_deviceResources->PIXEndEvent();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");

    // TODO: Pass the shadow map SRV here for sampling
    m_effect->SetCameraPosition(m_camera.GetPosition());
    m_effect->SetShadowMap(m_shadowMapSRV);
    m_effect->SetShadowTransform(GetShadowTransform());
    m_effect->SetView(m_camera.GetViewMatrix());
    m_effect->SetProjection(m_camera.GetProjectionMatrix());

    entityManager->DrawAll(m_effect.get());

    m_deviceResources->PIXEndEvent();

    m_physicsWindow.Draw();
    m_entityWindow.Draw();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Show the new frame.
    m_deviceResources->Present();
}

void Game::SetShadowMapPipelineState()
{
    m_deviceResources->PIXBeginEvent(L"SetShadowMapPipelineState");

    auto context = m_deviceResources->GetD3DDeviceContext();
    context->ClearDepthStencilView(m_shadowMapDSV.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.f, 0);

    ID3D11RenderTargetView* nullRTV = nullptr;

    context->OMSetRenderTargets(1, &nullRTV, m_shadowMapDSV.Get());
    context->RSSetViewports(1, &m_shadowMapViewport);

    m_deviceResources->PIXEndEvent();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, ColorsLinear::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    context->RSSetState(m_rsState.Get());

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
    textureManager->LoadWICTexture(m_deviceResources->GetD3DDevice(),
        "basketball",
        L"BasketballColor.jpg"
    );
    textureManager->LoadWICTexture(m_deviceResources->GetD3DDevice(),
        "softball",
        L"SoftballColor.jpg"
    );
    textureManager->LoadWICTexture(m_deviceResources->GetD3DDevice(),
        "crate",
        L"Wood_Crate_001_basecolor.jpg");

    auto deviceContext = m_deviceResources->GetD3DDeviceContext();
    JPH::BodyInterface& bodyInterface
        = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();


    // TODO: Don't create the physics objects here, they shouldn't be recreated if the device is lost

    Entity sphere1;
    sphere1.id = "sphere1";
    sphere1.Primitive = DirectX::GeometricPrimitive::CreateSphere(deviceContext, 2.f);
    sphere1.Texture = textureManager->GetTexture("basketball");
    sphere1.Translation = Matrix::CreateTranslation(Vector3{ -3.f, 3.f, 0.f });
    JPH::BodyCreationSettings sphere1Settings(
        new JPH::SphereShape(1.f),
        JPH::RVec3(-3.f, 3.f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::ObjectLayers::MOVING
    );

    sphere1Settings.mRestitution = 0.9f;
    sphere1Settings.mLinearVelocity = JPH::Vec3{ 1.5f, 0, 0 };
    sphere1.BodyID = bodyInterface.CreateAndAddBody(sphere1Settings, JPH::EActivation::Activate);

    entityManager->AddEntity(std::move(sphere1));

    Entity sphere2;
    sphere2.id = "sphere2";
    sphere2.Primitive = DirectX::GeometricPrimitive::CreateSphere(deviceContext, 2.f);
    sphere2.Texture = textureManager->GetTexture("softball");
    sphere2.Translation = Matrix::CreateTranslation(Vector3{ 3.f, 5.f, 0.f });
    JPH::BodyCreationSettings sphere2Settings(
        new JPH::SphereShape(1.f),
        JPH::RVec3(3.f, 5.f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::ObjectLayers::MOVING
    );
    sphere2Settings.mRestitution = 0.9f;
    sphere2.BodyID = bodyInterface.CreateAndAddBody(sphere2Settings, JPH::EActivation::Activate);

    entityManager->AddEntity(std::move(sphere2));

    Entity floor;
    floor.id = "floor";
    floor.Primitive = DirectX::GeometricPrimitive::CreateBox(deviceContext, Vector3{ 20.f, 0.5f, 20.f });
    floor.Translation = Matrix::CreateTranslation(Vector3{ 0.f, -0.25f, 0.f });

    JPH::BoxShape* floorShape = new JPH::BoxShape(JPH::Vec3(10.f, 0.25f, 10.f));
    JPH::BodyCreationSettings floorSettings(
        floorShape,
        JPH::RVec3(0.f, -0.25f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Gradient::Physics::ObjectLayers::NON_MOVING
    );

    auto floorBodyId = bodyInterface.CreateAndAddBody(floorSettings, JPH::EActivation::DontActivate);
    floor.BodyID = floorBodyId;
    entityManager->AddEntity(std::move(floor));

    Entity box1;
    box1.id = "box1";
    box1.Primitive = DirectX::GeometricPrimitive::CreateBox(deviceContext, Vector3{ 3.f, 3.f, 3.f });
    box1.Texture = textureManager->GetTexture("crate");
    box1.Translation = Matrix::CreateTranslation(Vector3{ -5.f, 1.5f, -4.f });
    JPH::BoxShape* box1Shape = new JPH::BoxShape(JPH::Vec3(1.5f, 1.5f, 1.5f));
    JPH::BodyCreationSettings box1Settings(
        box1Shape,
        JPH::RVec3(-5.f, 1.5f, -4.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Gradient::Physics::ObjectLayers::MOVING
    );
    auto box1BodyId = bodyInterface.CreateAndAddBody(box1Settings, JPH::EActivation::Activate);
    box1.BodyID = box1BodyId;
    entityManager->AddEntity(std::move(box1));
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    using namespace Gradient;

    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_shared<DirectX::CommonStates>(device);
    m_effect = std::make_unique<Effects::BlinnPhongEffect>(device, m_states);

    m_rsState = m_states->CullCounterClockwise();

    EntityManager::Initialize();
    TextureManager::Initialize(m_deviceResources->GetD3DDevice());

    CreateEntities();
    CreateShadowMapResources();
}

void Game::CreateShadowMapResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_shadowMapEffect = std::make_unique<Gradient::Effects::ShadowMapEffect>(device);

    const float sceneRadius = 15.f;
    SimpleMath::Vector3 lightDirection(-0.25f, -0.3f, 1.f);
    lightDirection.Normalize();
    auto lightPosition = -2 * sceneRadius * lightDirection;

    // TODO: Replace these with CreateShadow instead?
    m_shadowMapView = SimpleMath::Matrix::CreateLookAt(lightPosition,
        SimpleMath::Vector3::Zero,
        SimpleMath::Vector3::UnitY);

    m_shadowMapProj = SimpleMath::Matrix::CreateOrthographicOffCenter(
        -sceneRadius,
        sceneRadius,
        -sceneRadius,
        sceneRadius,
        sceneRadius,
        3 * sceneRadius
    );

    const float shadowMapWidth = 2048.f;

    m_shadowMapViewport = {
        0.0f,
        0.0f,
        shadowMapWidth,
        shadowMapWidth,
        0.f,
        1.f
    };

    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_R32_TYPELESS,
        (int)shadowMapWidth,
        (int)shadowMapWidth,
        1, // Use a single array entry.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
    );

    DX::ThrowIfFailed(device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        m_shadowMapDS.ReleaseAndGetAddressOf()
    ));

    CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(
        D3D11_DSV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_D32_FLOAT
    );

    DX::ThrowIfFailed(device->CreateDepthStencilView(
        m_shadowMapDS.Get(),
        &dsvDesc,
        m_shadowMapDSV.ReleaseAndGetAddressOf()
    ));

    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_shadowMapDS.Get(),
        D3D11_SRV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_R32_FLOAT);

    DX::ThrowIfFailed(device->CreateShaderResourceView(
        m_shadowMapDS.Get(),
        &srvDesc,
        m_shadowMapSRV.ReleaseAndGetAddressOf()
    ));

    auto rsDesc = CD3D11_RASTERIZER_DESC1();
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    rsDesc.ScissorEnable = FALSE;
    rsDesc.MultisampleEnable = FALSE;
    rsDesc.AntialiasedLineEnable = FALSE;
    rsDesc.ForcedSampleCount = 0;
    rsDesc.DepthBias = 10000;
    rsDesc.SlopeScaledDepthBias = 1.f;
    rsDesc.DepthBiasClamp = 0.f;

    DX::ThrowIfFailed(device->CreateRasterizerState1(&rsDesc,
        m_shadowMapRSState.ReleaseAndGetAddressOf()
    ));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto windowSize = m_deviceResources->GetOutputSize();
    m_camera.SetAspectRatio(windowSize.right / windowSize.bottom);
}

void Game::OnDeviceLost()
{
    auto entityManager = Gradient::EntityManager::Get();
    entityManager->OnDeviceLost();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
