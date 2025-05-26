#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/BufferStructs.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/RootSignature.h"
#include "Core/PipelineState.h"
#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/CommonStates.h>

namespace Gradient::Pipelines
{
    class PBRPipeline : public IRenderPipeline
    {
    public:
        static const size_t MAX_POINT_LIGHTS = 8;

        struct __declspec(align(256)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX worldViewProj;
        };

        struct __declspec(align(256)) PixelCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float tiling;
            DirectX::XMFLOAT3 emissiveRadiance;
            float pad2;
            DirectX::XMMATRIX shadowTransform;
        };

        struct __declspec(align(256)) LightCB
        {
            AlignedDirectionalLight directionalLight;
            AlignedPointLight pointLights[MAX_POINT_LIGHTS];
            uint32_t numPointLights;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit PBRPipeline(ID3D12Device2* device);
        virtual ~PBRPipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl,
            bool multisampled = true,
            DrawType passType = DrawType::PixelDepthReadWrite) override;

        virtual void SetMaterial(const Rendering::PBRMaterial& material) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
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
        void ApplyDepthOnlyPipeline(ID3D12GraphicsCommandList* cl,
            bool multisampled,
            DrawType passType);

        RootSignature m_rootSignature;
        std::unique_ptr<PipelineState> m_unmaskedPipelineState;
        std::unique_ptr<PipelineState> m_unmaskedShadowPipelineState;
        std::unique_ptr<PipelineState> m_unmaskedDepthWriteOnlyPSO;
        std::unique_ptr<PipelineState> m_unmaskedPixelDepthReadPSO;

        std::unique_ptr<PipelineState> m_maskedPipelineState;
        std::unique_ptr<PipelineState> m_maskedShadowPipelineState;
        std::unique_ptr<PipelineState> m_maskedDepthWriteOnlyPSO;
        std::unique_ptr<PipelineState> m_maskedPixelDepthReadPSO;

        Rendering::PBRMaterial m_material;

        GraphicsMemoryManager::DescriptorView m_shadowMap;
        GraphicsMemoryManager::DescriptorView m_environmentMap;
        GraphicsMemoryManager::DescriptorView m_shadowCubeArray;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        DirectX::SimpleMath::Vector3 m_cameraPosition;

        // TODO: Get rid of this and store Params::DirectionalLight
        // instead
        LightCB m_dLightCBData;

        std::vector<Params::PointLight> m_pointLights;
    };
}