#include "pch.h"

#include "Core/Rendering/Renderer.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/ECS/EntityManager.h"
#include "Core/ECS/Components/DrawableComponent.h"
#include "Core/ECS/Components/MaterialComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/PointLightComponent.h"
#include "Core/ECS/Components/InstanceDataComponent.h"
#include "Core/ECS/Components/RelationshipComponent.h"
#include "Core/TextureManager.h"
#include "Core/Physics/PhysicsEngine.h"
#include "Core/Math.h"

#include <imgui.h>
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx12.h"

namespace Gradient::Rendering
{

    // An integer version of ceil(value / divisor)
    template <typename T, typename U>
    T DivRoundUp(T value, U divisor)
    {
        return (value + divisor - 1) / divisor;
    }

    void Renderer::CreateWindowSizeIndependentResources(ID3D12Device2* device,
        ID3D12CommandQueue* cq)
    {
        auto bm = BufferManager::Get();

        m_states = std::make_unique<DirectX::CommonStates>(device);
        PbrPipeline = std::make_unique<Pipelines::PBRPipeline>(device);
        InstancePipeline = std::make_unique<Pipelines::InstancedPBRPipeline>(device);
        WaterPipeline = std::make_unique<Pipelines::WaterPipeline>(device);
        HeightmapPipeline = std::make_unique<Pipelines::HeightmapPipeline>(device);
        BillboardPipeline = std::make_unique<Pipelines::BillboardPipeline>(device);
        Tonemapper = std::make_unique<Rendering::TextureDrawer>(
            device,
            cq,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            L"ACESTonemapper_PS.cso"
        );

        auto dlight = new Gradient::Rendering::DirectionalLight(
            device,
            { -0.7f, -0.3f, 0.7f },
            200.f
        );
        DirectionalLight = std::unique_ptr<Gradient::Rendering::DirectionalLight>(dlight);
        auto lightColor = DirectX::SimpleMath::Color(0.86, 0.49, 0.06, 1);
        DirectionalLight->SetColour(lightColor);
        DirectionalLight->SetIrradiance(7.f);

        SkyDomePipeline = std::make_unique<Gradient::Pipelines::SkyDomePipeline>(device);
        SkyDomePipeline->SetAmbientIrradiance(1.f);
        SkyGeometry = bm->CreateGeoSphere(device,
            cq, 2.f, 3,
            false);
        EnvironmentMap = std::make_unique<Gradient::Rendering::CubeMap>(device,
            256,
            DXGI_FORMAT_R16G16B16A16_FLOAT);

        auto waterParams = Params::Water{
            50.f, 400.f
        };
        WaterPipeline->SetWaterParams(waterParams);

        // TODO: Don't hardcode the size
        ShadowCubeArray = std::make_unique<Rendering::DepthCubeArray>(device,
            256, 8);
    }

    void Renderer::CreateWindowSizeDependentResources(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        RECT windowSize)
    {
        auto width = static_cast<UINT>(windowSize.right);
        auto height = static_cast<UINT>(windowSize.bottom);

        MultisampledRT = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            width,
            height,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            true
        );

        BloomProcessor = std::make_unique<Gradient::Rendering::BloomProcessor>(
            device,
            cq,
            width,
            height,
            DXGI_FORMAT_R16G16B16A16_FLOAT
        );

        BloomProcessor->SetExposure(18.f);
        BloomProcessor->SetIntensity(0.3f);

        auto physicsEngine = Physics::PhysicsEngine::Get();
        physicsEngine->InitializeDebugRenderer(device, DXGI_FORMAT_R16G16B16A16_FLOAT);
    }

    std::vector<Gradient::Params::PointLight> Renderer::PointLightParams()
    {
        using namespace Gradient::ECS::Components;

        std::vector<Gradient::Params::PointLight> out;
        auto em = Gradient::EntityManager::Get();
        auto view = em->Registry.view<TransformComponent, PointLightComponent>();

        for (auto entity : view)
        {
            auto [transform, light] = view.get(entity);

            auto params = light.PointLight.AsParams();
            params.Position = transform.GetTranslation();

            out.push_back(params);
        }

        return out;
    }

    void Renderer::Clear(ID3D12GraphicsCommandList* cl,
        D3D12_VIEWPORT screenViewport)
    {
        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Clear");

        MultisampledRT->Clear(cl);
        cl->RSSetViewports(1, &screenViewport);

        PIXEndEvent(cl);
    }

    void Renderer::SetShadowViewProj(const Camera* camera,
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& proj)
    {
        HeightmapPipeline->SetCameraPosition(camera->GetPosition());
        HeightmapPipeline->SetCameraDirection(camera->GetDirection());
        BillboardPipeline->CameraPosition = camera->GetPosition();
        BillboardPipeline->CameraDirection = camera->GetDirection();

        PbrPipeline->SetView(DirectionalLight->GetView());
        PbrPipeline->SetProjection(DirectionalLight->GetProjection());
        InstancePipeline->SetView(DirectionalLight->GetView());
        InstancePipeline->SetProjection(DirectionalLight->GetProjection());
        BillboardPipeline->View = DirectionalLight->GetView();
        BillboardPipeline->Proj = DirectionalLight->GetProjection();
        HeightmapPipeline->SetView(DirectionalLight->GetView());
        HeightmapPipeline->SetProjection(DirectionalLight->GetProjection());
    }

    void Renderer::SetFrameParameters(const Camera* camera)
    {
        PbrPipeline->SetCameraPosition(camera->GetPosition());
        PbrPipeline->SetDirectionalLight(DirectionalLight.get());
        PbrPipeline->SetView(camera->GetViewMatrix());
        PbrPipeline->SetProjection(camera->GetProjectionMatrix());
        PbrPipeline->SetEnvironmentMap(EnvironmentMap->GetSRV());
        PbrPipeline->SetPointLights(PointLightParams());
        PbrPipeline->SetShadowCubeArray(ShadowCubeArray->GetSRV());

        InstancePipeline->SetCameraPosition(camera->GetPosition());
        InstancePipeline->SetDirectionalLight(DirectionalLight.get());
        InstancePipeline->SetView(camera->GetViewMatrix());
        InstancePipeline->SetProjection(camera->GetProjectionMatrix());
        InstancePipeline->SetEnvironmentMap(EnvironmentMap->GetSRV());
        InstancePipeline->SetPointLights(PointLightParams());
        InstancePipeline->SetShadowCubeArray(ShadowCubeArray->GetSRV());

        BillboardPipeline->CameraPosition = camera->GetPosition();
        BillboardPipeline->CameraDirection = camera->GetDirection();
        BillboardPipeline->View = camera->GetViewMatrix();
        BillboardPipeline->Proj = camera->GetProjectionMatrix();
        BillboardPipeline->SetDirectionalLight(DirectionalLight.get());
        BillboardPipeline->EnvironmentMap = EnvironmentMap->GetSRV();
        BillboardPipeline->ShadowCubeArray = ShadowCubeArray->GetSRV();

        HeightmapPipeline->SetCameraPosition(camera->GetPosition());
        HeightmapPipeline->SetCameraDirection(camera->GetDirection());
        HeightmapPipeline->SetDirectionalLight(DirectionalLight.get());
        HeightmapPipeline->SetView(camera->GetViewMatrix());
        HeightmapPipeline->SetProjection(camera->GetProjectionMatrix());
        HeightmapPipeline->SetEnvironmentMap(EnvironmentMap->GetSRV());
        HeightmapPipeline->SetPointLights(PointLightParams());
        HeightmapPipeline->SetShadowCubeArray(ShadowCubeArray->GetSRV());

        WaterPipeline->SetCameraPosition(camera->GetPosition());
        WaterPipeline->SetCameraDirection(camera->GetDirection());
        WaterPipeline->SetDirectionalLight(DirectionalLight.get());
        WaterPipeline->SetView(camera->GetViewMatrix());
        WaterPipeline->SetProjection(camera->GetProjectionMatrix());
        WaterPipeline->SetEnvironmentMap(EnvironmentMap->GetSRV());
        WaterPipeline->SetPointLights(PointLightParams());
        WaterPipeline->SetShadowCubeArray(ShadowCubeArray->GetSRV());
    }

    void Renderer::Render(ID3D12GraphicsCommandList6* cl,
        D3D12_VIEWPORT screenViewport,
        Camera* camera,
        RenderTexture* finalRenderTarget)
    {
        using namespace DirectX;
        using namespace Gradient::ECS::Components;

        auto entityManager = Gradient::EntityManager::Get();
        auto gmm = Gradient::GraphicsMemoryManager::Get();
        auto bm = BufferManager::Get();

        ID3D12DescriptorHeap* heaps[] = { gmm->GetSrvDescriptorHeap(), m_states->Heap() };
        cl->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

        D3D12_RECT scissorRect;
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = LONG_MAX; // Large value ensures no clipping
        scissorRect.bottom = LONG_MAX;

        cl->RSSetScissorRects(1, &scissorRect);

        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Shadow Pass");

        DirectionalLight->SetCameraFrustum(camera->GetShadowFrustum());
        DirectionalLight->ClearAndSetDSV(cl);

        SetShadowViewProj(camera,
            DirectionalLight->GetView(),
            DirectionalLight->GetProjection());

        DrawAllEntities(cl, PassType::ShadowPass, camera->GetFrustum(), DirectionalLight->GetShadowBB());

        auto pointLightsView = entityManager->Registry.view<TransformComponent, PointLightComponent>();
        for (auto& entity : pointLightsView)
        {
            auto [transform, light] = pointLightsView.get(entity);
            // TODO: Cull lights whose range doesn't intersect 
            // with the camera frustum.
            ShadowCubeArray->Render(cl,
                light.PointLight.ShadowCubeIndex,
                transform.GetTranslation(),
                light.PointLight.MinRange,
                light.PointLight.MaxRange,
                [=](SimpleMath::Matrix view, SimpleMath::Matrix proj)
                {
                    SetShadowViewProj(camera, view, proj);

                    auto frustum = Math::MakeFrustum(view, proj);

                    DrawAllEntities(cl, PassType::ShadowPass, frustum);
                });
        }

        PIXEndEvent(cl);

        SkyDomePipeline->SetDirectionalLight(DirectionalLight.get());

        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Environment Map");

        EnvironmentMap->Render(cl,
            [=](SimpleMath::Matrix view, SimpleMath::Matrix proj)
            {
                SkyDomePipeline->SetSunCircleEnabled(false);
                SkyDomePipeline->SetProjection(proj);
                SkyDomePipeline->SetView(view);
                SkyDomePipeline->Apply(cl, false);
                bm->GetMesh(SkyGeometry)->Draw(cl);
            });

        PIXEndEvent(cl);

        Clear(cl, screenViewport);


        SkyDomePipeline->SetSunCircleEnabled(true);
        SkyDomePipeline->SetProjection(camera->GetProjectionMatrix());
        SkyDomePipeline->SetView(camera->GetViewMatrix());


        // TODO: Batch these barrier transitions
        DirectionalLight->TransitionToShaderResource(cl);
        EnvironmentMap->TransitionToShaderResource(cl);
        ShadowCubeArray->TransitionToShaderResource(cl);

        SetFrameParameters(camera);

        // The Z pre-pass is not worth it for the time being

        //PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Z-prepass");
        //MultisampledRT->SetDepthOnly(cl);
        //m_prepassedEntities.clear();

        //DrawAllEntities(cl, PassType::ZPrepass, camera->GetPrepassFrustum());

        //PIXEndEvent(cl);


        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Forward pass");
        MultisampledRT->SetDepthAndRT(cl);

        SkyDomePipeline->Apply(cl);
        bm->GetMesh(SkyGeometry)->Draw(cl);

        DrawAllEntities(cl, PassType::ForwardPass, camera->GetFrustum());

        auto physicsEngine = Physics::PhysicsEngine::Get();
        // This is too slow for now and is hence commented out
        //physicsEngine->DebugDrawBodies(cl,
        //    camera->GetViewMatrix(),
        //    camera->GetProjectionMatrix(),
        //    camera->GetPosition());

        MultisampledRT->CopyToSingleSampled(cl);
        PIXEndEvent(cl);

        // Post-process ----
        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Bloom");
        auto bloomOutput = BloomProcessor->Process(cl,
            MultisampledRT.get(),
            screenViewport);
        PIXEndEvent(cl);

        // Tonemap
        bloomOutput->DrawTo(cl,
            finalRenderTarget,
            Tonemapper.get(),
            screenViewport);
    }

    void Renderer::DrawAllEntities(ID3D12GraphicsCommandList6* cl,
        PassType passType,
        std::optional<DirectX::BoundingFrustum> viewFrustum,
        std::optional<DirectX::BoundingOrientedBox> shadowBB)
    {
        using namespace ECS::Components;
        auto em = EntityManager::Get();
        auto bm = BufferManager::Get();

        // Default shading model without instancing
        auto defaultView = em->Registry.view<DrawableComponent,
            TransformComponent,
            MaterialComponent>(entt::exclude<InstanceDataComponent>);
        for (auto entity : defaultView)
        {
            auto [drawable, transform, material] = defaultView.get(entity);

            auto mesh = bm->GetMesh(drawable.MeshHandle);

            if (mesh == nullptr) continue;

            if (passType == PassType::ShadowPass
                && !drawable.CastsShadows) continue;

            // TODO: Support the z pre-pass for non-instanced entities
            if (passType == PassType::ZPrepass) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Default)
                continue;

            auto bb = em->GetBoundingBox(entity);
            if (passType == PassType::ShadowPass
                && shadowBB && bb)
            {
                if (!shadowBB.value().Intersects(bb.value()))
                {
                    continue;
                }
            }
            else  if (viewFrustum && bb)
            {
                if (!viewFrustum.value().Intersects(bb.value()))
                {
                    continue;
                }
            }

            PbrPipeline->SetMaterial(material.Material);

            auto world = em->GetWorldMatrix(entity);
            PbrPipeline->SetWorld(world);

            auto drawType = passType == PassType::ShadowPass ? DrawType::ShadowPass : DrawType::PixelDepthReadWrite;

            PbrPipeline->Apply(cl, true, drawType);

            mesh->Draw(cl);
        }

        // Default shading model with instancing
        auto instanceView = em->Registry.view<DrawableComponent,
            TransformComponent,
            MaterialComponent,
            InstanceDataComponent>();
        for (auto entity : instanceView)
        {
            auto [drawable, transform, material, instances] = instanceView.get(entity);
            auto mesh = bm->GetMesh(drawable.MeshHandle);

            if (mesh == nullptr) continue;

            if (passType == PassType::ShadowPass
                && !drawable.CastsShadows) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Default)
                continue;

            auto bb = em->GetBoundingBox(entity);
            if (passType == PassType::ShadowPass
                && shadowBB && bb)
            {
                if (!shadowBB.value().Intersects(bb.value()))
                {
                    continue;
                }
            }
            else if (viewFrustum && bb)
            {
                if (!viewFrustum.value().Intersects(bb.value()))
                {
                    continue;
                }
            }

            InstancePipeline->SetMaterial(material.Material);
            InstancePipeline->SetWorld(em->GetWorldMatrix(entity));
            InstancePipeline->SetInstanceData(instances);

            DrawType drawType;

            switch (passType)
            {
            case PassType::ShadowPass:
                drawType = DrawType::ShadowPass;
                break;

            case PassType::ZPrepass:
                drawType = DrawType::DepthWriteOnly;
                m_prepassedEntities.insert(entity);
                break;

            case PassType::ForwardPass:
                if (m_prepassedEntities.contains(entity))
                {
                    drawType = DrawType::PixelDepthReadOnly;
                }
                else
                {
                    drawType = DrawType::PixelDepthReadWrite;
                }
                break;

            default:
                drawType = DrawType::PixelDepthReadWrite;
                break;
            }

            InstancePipeline->Apply(cl, true, drawType);

            auto bufferEntry = bm->GetInstanceBuffer(instances.BufferHandle);

            if (bufferEntry)
            {
                mesh->Draw(cl, bufferEntry->InstanceCount);
            }
        }

        // Billboard shading model with instancing
        auto billboardView = em->Registry.view<DrawableComponent,
            TransformComponent,
            MaterialComponent,
            InstanceDataComponent>();
        for (auto entity : billboardView)
        {
            auto [drawable, transform, material, instances] = billboardView.get(entity);

            if (passType == PassType::ShadowPass
                && !drawable.CastsShadows) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Billboard)
                continue;

            auto bb = em->GetBoundingBox(entity);
            if (passType == PassType::ShadowPass
                && shadowBB && bb)
            {
                if (!shadowBB.value().Intersects(bb.value()))
                {
                    continue;
                }
            }
            else if (viewFrustum && bb)
            {
                if (!viewFrustum.value().Intersects(bb.value()))
                {
                    continue;
                }
            }

            BillboardPipeline->Material = material.Material;
            BillboardPipeline->World = em->GetWorldMatrix(entity);
            BillboardPipeline->InstanceHandle = instances.BufferHandle;

            DrawType drawType;

            switch (passType)
            {
            case PassType::ShadowPass:
                drawType = DrawType::ShadowPass;
                break;

            case PassType::ZPrepass:
                drawType = DrawType::DepthWriteOnly;
                m_prepassedEntities.insert(entity);
                break;

            case PassType::ForwardPass:
                if (m_prepassedEntities.contains(entity))
                {
                    drawType = DrawType::PixelDepthReadOnly;
                }
                else
                {
                    drawType = DrawType::PixelDepthReadWrite;
                }
                break;

            default:
                drawType = DrawType::PixelDepthReadWrite;
                break;
            }

            auto bufferEntry = bm->GetInstanceBuffer(instances.BufferHandle);

            if (bufferEntry)
            {
                BillboardPipeline->CardDimensions = drawable.BillboardDimensions;
                BillboardPipeline->InstanceCount = bufferEntry->InstanceCount;
                BillboardPipeline->Apply(cl, true, drawType);

                uint32_t foo = DivRoundUp(bufferEntry->InstanceCount, 4u);

                cl->DispatchMesh(DivRoundUp(bufferEntry->InstanceCount, 4u), 1, 1);
            }
        }

        // Heightmap shading model
        auto heightmapView = em->Registry.view<DrawableComponent,
            TransformComponent,
            HeightMapComponent,
            MaterialComponent>();
        for (auto entity : heightmapView)
        {
            auto [drawable, transform, heightMap, material] = heightmapView.get(entity);
            auto mesh = bm->GetMesh(drawable.MeshHandle);

            if (mesh == nullptr) continue;
            if (passType == PassType::ShadowPass
                && !drawable.CastsShadows) continue;

            if (passType == PassType::ZPrepass) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Heightmap)
                continue;

            auto drawType = passType == PassType::ShadowPass ? DrawType::ShadowPass : DrawType::PixelDepthReadWrite;

            HeightmapPipeline->SetMaterial(material.Material);
            HeightmapPipeline->SetHeightMapComponent(heightMap);
            HeightmapPipeline->SetWorld(em->GetWorldMatrix(entity));
            HeightmapPipeline->Apply(cl, true, drawType);

            mesh->Draw(cl);
        }

        // Water shading model
        auto waterView = em->Registry.view<DrawableComponent,
            TransformComponent>();
        for (auto entity : waterView)
        {
            auto [drawable, transform] = waterView.get(entity);
            auto mesh = bm->GetMesh(drawable.MeshHandle);

            if (mesh == nullptr) continue;
            if (passType != PassType::ForwardPass) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Water)
                continue;

            WaterPipeline->SetWorld(em->GetWorldMatrix(entity));
            WaterPipeline->Apply(cl, true, DrawType::PixelDepthReadWrite);

            mesh->Draw(cl);
        }
    }
}