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

#include <imgui.h>
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx12.h"

namespace Gradient::Rendering
{

    void Renderer::CreateWindowSizeIndependentResources(ID3D12Device* device,
        ID3D12CommandQueue* cq)
    {
        m_states = std::make_unique<DirectX::CommonStates>(device);
        PbrPipeline = std::make_unique<Pipelines::PBRPipeline>(device);
        InstancePipeline = std::make_unique<Pipelines::InstancedPBRPipeline>(device);
        WaterPipeline = std::make_unique<Pipelines::WaterPipeline>(device);
        HeightmapPipeline = std::make_unique<Pipelines::HeightmapPipeline>(device);
        Tonemapper = std::make_unique<Rendering::TextureDrawer>(
            device,
            cq,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            L"ACESTonemapper_PS.cso"
        );

        auto dlight = new Gradient::Rendering::DirectionalLight(
            device,
            { -0.7f, -0.3f, 0.7f },
            100.f
        );
        DirectionalLight = std::unique_ptr<Gradient::Rendering::DirectionalLight>(dlight);
        auto lightColor = DirectX::SimpleMath::Color(0.86, 0.49, 0.06, 1);
        DirectionalLight->SetColour(lightColor);
        DirectionalLight->SetIrradiance(7.f);

        SkyDomePipeline = std::make_unique<Gradient::Pipelines::SkyDomePipeline>(device);
        SkyDomePipeline->SetAmbientIrradiance(1.f);
        SkyGeometry = Rendering::ProceduralMesh::CreateGeoSphere(device,
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

        MultisampledRT->ClearAndSetAsTarget(cl);
        cl->RSSetViewports(1, &screenViewport);

        PIXEndEvent(cl);
    }

    void Renderer::Render(ID3D12GraphicsCommandList* cl,
        D3D12_VIEWPORT screenViewport,
        Camera* camera,
        RenderTexture* finalRenderTarget)
    {
        using namespace DirectX;
        using namespace Gradient::ECS::Components;

        auto entityManager = Gradient::EntityManager::Get();
        auto gmm = Gradient::GraphicsMemoryManager::Get();

        ID3D12DescriptorHeap* heaps[] = { gmm->GetSrvDescriptorHeap(), m_states->Heap() };
        cl->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

        D3D12_RECT scissorRect;
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = LONG_MAX; // Large value ensures no clipping
        scissorRect.bottom = LONG_MAX;

        cl->RSSetScissorRects(1, &scissorRect);

        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Shadow Pass");

        DirectionalLight->ClearAndSetDSV(cl);

        HeightmapPipeline->SetCameraPosition(camera->GetPosition());
        HeightmapPipeline->SetCameraDirection(camera->GetDirection());

        PbrPipeline->SetView(DirectionalLight->GetView());
        PbrPipeline->SetProjection(DirectionalLight->GetProjection());
        InstancePipeline->SetView(DirectionalLight->GetView());
        InstancePipeline->SetProjection(DirectionalLight->GetProjection());
        HeightmapPipeline->SetView(DirectionalLight->GetView());
        HeightmapPipeline->SetProjection(DirectionalLight->GetProjection());


        DrawAllEntities(cl, true);

        auto pointLightsView = entityManager->Registry.view<TransformComponent, PointLightComponent>();
        for (auto& entity : pointLightsView)
        {
            auto [transform, light] = pointLightsView.get(entity);
            ShadowCubeArray->Render(cl,
                light.PointLight.ShadowCubeIndex,
                transform.GetTranslation(),
                light.PointLight.MinRange,
                light.PointLight.MaxRange,
                [=](SimpleMath::Matrix view, SimpleMath::Matrix proj)
                {
                    PbrPipeline->SetView(view);
                    PbrPipeline->SetProjection(proj);
                    HeightmapPipeline->SetView(view);
                    HeightmapPipeline->SetProjection(proj);

                    // TODO: Cull entities that are not in the light's range
                    DrawAllEntities(cl, true);
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
                SkyGeometry->Draw(cl);
            });

        PIXEndEvent(cl);

        Clear(cl, screenViewport);

        PIXBeginEvent(cl, PIX_COLOR_DEFAULT, L"Render");
        SkyDomePipeline->SetSunCircleEnabled(true);
        SkyDomePipeline->SetProjection(camera->GetProjectionMatrix());
        SkyDomePipeline->SetView(camera->GetViewMatrix());
        SkyDomePipeline->Apply(cl);
        SkyGeometry->Draw(cl);

        // TODO: Batch these barrier transitions
        DirectionalLight->TransitionToShaderResource(cl);
        EnvironmentMap->TransitionToShaderResource(cl);
        ShadowCubeArray->TransitionToShaderResource(cl);

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

        DrawAllEntities(cl);

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

    void Renderer::DrawAllEntities(ID3D12GraphicsCommandList* cl,
        bool drawingShadows)
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
            auto [drawable, transform, material]
                = defaultView.get(entity);

            if (drawable.Drawable == nullptr) continue;

            if (drawingShadows && !drawable.CastsShadows) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Default)
                continue;

            if (!drawingShadows)
                PbrPipeline->SetMaterial(material.Material);

            PbrPipeline->SetWorld(em->GetWorldMatrix(entity));
            PbrPipeline->Apply(cl, true, drawingShadows);

            drawable.Drawable->Draw(cl);
        }

        // Default shading model with instancing
        auto instanceView = em->Registry.view<DrawableComponent,
            TransformComponent,
            MaterialComponent,
            InstanceDataComponent>();
        for (auto entity : instanceView)
        {
            auto [drawable, transform, material, instances]
                = instanceView.get(entity);

                if (drawable.Drawable == nullptr) continue;

                if (drawingShadows && !drawable.CastsShadows) continue;

                if (drawable.ShadingModel
                    != DrawableComponent::ShadingModel::Default)
                    continue;

                if (!drawingShadows)
                    InstancePipeline->SetMaterial(material.Material);

                InstancePipeline->SetWorld(em->GetWorldMatrix(entity));
                InstancePipeline->SetInstanceData(instances);
                InstancePipeline->Apply(cl, true, drawingShadows);

                auto bufferEntry = bm->GetInstanceBuffer(instances.BufferHandle);

                if (bufferEntry)
                {
                    drawable.Drawable->Draw(cl, bufferEntry.value().InstanceCount);
                }
        }

        // Heightmap shading model
        auto heightmapView = em->Registry.view<DrawableComponent,
            TransformComponent,
            HeightMapComponent>();
        for (auto entity : heightmapView)
        {
            auto [drawable, transform, heightMap] = heightmapView.get(entity);

            if (drawable.Drawable == nullptr) continue;
            if (drawingShadows && !drawable.CastsShadows) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Heightmap)
                continue;

            HeightmapPipeline->SetHeightMapComponent(heightMap);
            HeightmapPipeline->SetWorld(em->GetWorldMatrix(entity));
            HeightmapPipeline->Apply(cl, true, drawingShadows);

            drawable.Drawable->Draw(cl);
        }

        // Water shading model
        auto waterView = em->Registry.view<DrawableComponent,
            TransformComponent>();
        for (auto entity : waterView)
        {
            auto [drawable, transform] = waterView.get(entity);

            if (drawable.Drawable == nullptr) continue;
            if (drawingShadows) continue;

            if (drawable.ShadingModel
                != DrawableComponent::ShadingModel::Water)
                continue;

            WaterPipeline->SetWorld(em->GetWorldMatrix(entity));
            WaterPipeline->Apply(cl, true, drawingShadows);

            drawable.Drawable->Draw(cl);
        }
    }
}