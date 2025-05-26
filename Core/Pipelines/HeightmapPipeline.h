#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/BufferStructs.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Parameters.h"
#include "Core/RootSignature.h"
#include "Core/PipelineState.h"
#include "Core/ECS/Components/HeightMapComponent.h"
#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/CommonStates.h>
#include <array>

namespace Gradient::Pipelines
{
    class HeightmapPipeline : public IRenderPipeline
    {
    public:
        static const size_t MAX_POINT_LIGHTS = 8;

        struct __declspec(align(256)) MatrixCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct __declspec(align(256)) LodCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float minLodDistance;
            DirectX::XMFLOAT3 cameraDirection;
            float maxLodDistance;
            uint32_t cullingEnabled = 1;
            float pad[3];
        };

        struct __declspec(align(256)) HeightMapParamsCB
        {
            float height;
            float gridWidth;
            float pad[2];
        };

        struct __declspec(align(256)) LightCB
        {
            AlignedDirectionalLight directionalLight;
            AlignedPointLight pointLights[MAX_POINT_LIGHTS];
            uint32_t numPointLights;
        };

        struct __declspec(align(256)) PixelParamCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float tiling;
            DirectX::XMMATRIX shadowTransform;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit HeightmapPipeline(ID3D12Device2* device);
        virtual ~HeightmapPipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl, 
            bool multisampled = true,
            DrawType passType = DrawType::PixelDepthReadWrite) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

        void SetHeightMapComponent(const ECS::Components::HeightMapComponent& hmComponent);
        void SetMaterial(const Rendering::PBRMaterial& material) override;


        // TODO: Extract all of this junk into a single struct
        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
        void SetCameraDirection(DirectX::SimpleMath::Vector3 cameraDirection);
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);
        void SetPointLights(std::vector<Params::PointLight> pointLights);
        void SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index);
        void SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index);
        GraphicsMemoryManager::DescriptorView GTAOTexture;

    private:
        void InitializeRootSignature(ID3D12Device* device);
        void InitializeShadowPSO(ID3D12Device2* device);
        void InitializeRenderPSO(ID3D12Device2* device);
        void InitializeDepthWritePSO(ID3D12Device2* device);
        void InitializePixelDepthReadPSO(ID3D12Device2* device);
        void ApplyShadowPipeline(ID3D12GraphicsCommandList* cl);
        void ApplyDepthWriteOnlyPipeline(ID3D12GraphicsCommandList* cl, bool multisampled);

        RootSignature m_rootSignature;
        std::unique_ptr<PipelineState> m_pso;
        std::unique_ptr<PipelineState> m_shadowPso;
        std::unique_ptr<PipelineState> m_depthWritePso;
        std::unique_ptr<PipelineState> m_pixelDepthReadPso;

        GraphicsMemoryManager::DescriptorView m_shadowMap;
        GraphicsMemoryManager::DescriptorView m_environmentMap;
        GraphicsMemoryManager::DescriptorView m_shadowCubeArray;
        
        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        Rendering::PBRMaterial m_material;

        DirectX::SimpleMath::Vector3 m_cameraPosition;
        DirectX::SimpleMath::Vector3 m_cameraDirection;

        // TODO: Store a Params::DirectionalLight instead.
        DirectX::SimpleMath::Color   m_directionalLightColour;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        float m_lightIrradiance;

        std::vector<Params::PointLight> m_pointLights;

        ECS::Components::HeightMapComponent m_heightMapComponent;
    };
}