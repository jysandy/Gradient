#pragma once

#include "pch.h"
#include "Core/BarrierResource.h"
#include "Core/Rendering/RenderTexture.h"
#include "Core/Pipelines/PBRPipeline.h"
#include "Core/Pipelines/SkyDomePipeline.h"
#include "Core/Pipelines/WaterPipeline.h"
#include "Core/Pipelines/HeightmapPipeline.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/Rendering/BloomProcessor.h"
#include "Core/Rendering/CubeMap.h"
#include "Core/Rendering/GeometricPrimitive.h"
#include "Core/Camera.h"

namespace Gradient::Rendering
{
    class Renderer
    {
    public:
        Renderer() = default;

        void CreateWindowSizeIndependentResources(ID3D12Device* device,
            ID3D12CommandQueue* cq);
        void CreateWindowSizeDependentResources(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            RECT windowSize);

        void Clear(ID3D12GraphicsCommandList* cl, D3D12_VIEWPORT screenViewport);
        std::vector<Gradient::Params::PointLight> PointLightParams();

        void Render(ID3D12GraphicsCommandList* cl,
            D3D12_VIEWPORT screenViewport,
            Camera* camera,
            RenderTexture* finalRenderTarget);

        void DrawAllEntities(ID3D12GraphicsCommandList* cl,
            bool drawingShadows = false);

        std::unique_ptr<DirectX::CommonStates> m_states;

        std::unique_ptr<Gradient::Pipelines::PBRPipeline> PbrPipeline;
        std::unique_ptr<Gradient::Pipelines::WaterPipeline> WaterPipeline;
        std::unique_ptr<Gradient::Pipelines::HeightmapPipeline> HeightmapPipeline;
        std::unique_ptr<Gradient::Pipelines::SkyDomePipeline> SkyDomePipeline;

        std::unique_ptr<Gradient::Rendering::RenderTexture> MultisampledRT;
        std::unique_ptr<Gradient::Rendering::BloomProcessor> BloomProcessor;

        std::unique_ptr<Gradient::Rendering::GeometricPrimitive> SkyGeometry;
        std::unique_ptr<Gradient::Rendering::CubeMap> EnvironmentMap;
        std::unique_ptr<Gradient::Rendering::TextureDrawer> Tonemapper;

        std::unique_ptr<Gradient::Rendering::DirectionalLight> DirectionalLight;
        std::unique_ptr<Gradient::Rendering::DepthCubeArray> ShadowCubeArray;
    };
}