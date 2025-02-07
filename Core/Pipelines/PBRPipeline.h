#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/BufferStructs.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/RootSignature.h"
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

        struct __declspec(align(16)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct __declspec(align(16)) PixelCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float pad;
            DirectX::XMFLOAT3 emissiveRadiance;
            float pad2;
            DirectX::XMMATRIX shadowTransform;
        };

        struct __declspec(align(16)) LightCB
        {
            AlignedDirectionalLight directionalLight;
            AlignedPointLight pointLights[MAX_POINT_LIGHTS];
            uint32_t numPointLights;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit PBRPipeline(ID3D12Device* device);
        virtual ~PBRPipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl, bool multisampled = true) override;

        virtual void SetAlbedo(GraphicsMemoryManager::DescriptorView index) override;
        virtual void SetNormalMap(GraphicsMemoryManager::DescriptorView index) override;
        virtual void SetAOMap(GraphicsMemoryManager::DescriptorView index) override;
        virtual void SetMetalnessMap(GraphicsMemoryManager::DescriptorView index) override;
        virtual void SetRoughnessMap(GraphicsMemoryManager::DescriptorView index) override;
        virtual void SetEmissiveRadiance(DirectX::SimpleMath::Vector3 radiance) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);
        void SetPointLights(std::vector<Params::PointLight> pointLights);
        void SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index);
        void SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index);

    private:
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_multisampledPSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_singleSampledPSO;
        RootSignature m_rootSignature;

        GraphicsMemoryManager::DescriptorView m_texture;
        GraphicsMemoryManager::DescriptorView m_shadowMap;
        GraphicsMemoryManager::DescriptorView m_normalMap;
        GraphicsMemoryManager::DescriptorView m_aoMap;
        GraphicsMemoryManager::DescriptorView m_metalnessMap;
        GraphicsMemoryManager::DescriptorView m_roughnessMap;
        GraphicsMemoryManager::DescriptorView m_environmentMap;
        GraphicsMemoryManager::DescriptorView m_shadowCubeArray;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        DirectX::SimpleMath::Vector3 m_cameraPosition;
        DirectX::SimpleMath::Vector3 m_emissiveRadiance = { 0, 0, 0 };
        
        // TODO: Get rid of this and store Params::DirectionalLight
        // instead
        LightCB m_dLightCBData;

        std::vector<Params::PointLight> m_pointLights;
    };
}