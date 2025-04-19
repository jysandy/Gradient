#pragma once

#include "pch.h"
#include "Core/BarrierResource.h"
#include "Core/Rendering/RenderTexture.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/PBRPipeline.h"
#include "Core/Pipelines/InstancedPBRPipeline.h"
#include "Core/Pipelines/SkyDomePipeline.h"
#include "Core/Pipelines/WaterPipeline.h"
#include "Core/Pipelines/HeightmapPipeline.h"
#include "Core/Pipelines/BillboardPipeline.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/Rendering/BloomProcessor.h"
#include "Core/Rendering/CubeMap.h"
#include "Core/Rendering/ProceduralMesh.h"
#include "Core/BufferManager.h"
#include "Core/Camera.h"

#include <entt/entt.hpp>

#include <set>

namespace Gradient::Rendering
{
    class Renderer
    {
    public:
        using DrawType = Pipelines::IRenderPipeline::DrawType;
        enum class PassType
        {
            ShadowPass,
            ZPrepass,
            ForwardPass
        };

        Renderer() = default;

        void CreateWindowSizeIndependentResources(ID3D12Device2* device,
            ID3D12CommandQueue* cq);
        void CreateWindowSizeDependentResources(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            RECT windowSize);

        void Clear(ID3D12GraphicsCommandList* cl, D3D12_VIEWPORT screenViewport);
        std::vector<Gradient::Params::PointLight> PointLightParams();

        void Render(ID3D12GraphicsCommandList6* cl,
            D3D12_VIEWPORT screenViewport,
            Camera* camera,
            RenderTexture* finalRenderTarget);

        void DrawAllEntities(ID3D12GraphicsCommandList6* cl,
            PassType passType = PassType::ForwardPass,
            std::optional<DirectX::BoundingFrustum> viewFrustum = std::nullopt,
            std::optional<DirectX::BoundingOrientedBox> shadowBB = std::nullopt);

        void SetShadowViewProj(const Camera* camera, 
            const DirectX::SimpleMath::Matrix& view, 
            const DirectX::SimpleMath::Matrix& proj);

        void SetFrameParameters(const Camera* camera);

        std::unique_ptr<DirectX::CommonStates> m_states;

        std::unique_ptr<Gradient::Pipelines::PBRPipeline> PbrPipeline;
        std::unique_ptr<Gradient::Pipelines::InstancedPBRPipeline> InstancePipeline;
        std::unique_ptr<Gradient::Pipelines::WaterPipeline> WaterPipeline;
        std::unique_ptr<Gradient::Pipelines::HeightmapPipeline> HeightmapPipeline;
        std::unique_ptr<Gradient::Pipelines::SkyDomePipeline> SkyDomePipeline;
        std::unique_ptr<Gradient::Pipelines::BillboardPipeline> BillboardPipeline;

        std::unique_ptr<Gradient::Rendering::RenderTexture> MultisampledRT;
        std::unique_ptr<Gradient::Rendering::BloomProcessor> BloomProcessor;

        BufferManager::MeshHandle SkyGeometry;
        std::unique_ptr<Gradient::Rendering::CubeMap> EnvironmentMap;
        std::unique_ptr<Gradient::Rendering::TextureDrawer> Tonemapper;

        std::unique_ptr<Gradient::Rendering::DirectionalLight> DirectionalLight;
        std::unique_ptr<Gradient::Rendering::DepthCubeArray> ShadowCubeArray;

        std::set<entt::entity> m_prepassedEntities;
    };
}