#pragma once

#include "pch.h"

#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/BufferStructs.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Rendering/PointLight.h"
#include "Core/RootSignature.h"
#include "Core/PipelineState.h"
#include "Core/ECS/Components/InstanceDataComponent.h"
#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/CommonStates.h>

namespace Gradient::Pipelines
{
    class BillboardPipeline : public IRenderPipeline
    {
    public:
        static const size_t MAX_POINT_LIGHTS = 8;

        struct __declspec(align(16)) MatrixCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX viewProj;
        };

        struct __declspec(align(16)) DrawParamsCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float cardWidth;
            DirectX::XMFLOAT3 cameraDirection;
            float cardHeight;
            uint32_t numInstances;
            uint32_t useCameraDirectionForCulling = 0;
            float pad[2];
        };

        struct __declspec(align(16)) PixelCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float tiling;
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

        explicit BillboardPipeline(ID3D12Device2* device);
        virtual ~BillboardPipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl,
            bool multisampled = true,
            DrawType passType = DrawType::PixelDepthReadWrite) override;

        const uint32_t InstancesPerThreadGroup = 32u;

        Rendering::PBRMaterial Material;

        GraphicsMemoryManager::DescriptorView EnvironmentMap;

        BufferManager::InstanceBufferHandle InstanceHandle;
        uint32_t InstanceCount;
        DirectX::XMFLOAT2 CardDimensions;

        DirectX::SimpleMath::Matrix World;
        DirectX::SimpleMath::Matrix View;
        DirectX::SimpleMath::Matrix Proj;

        DirectX::SimpleMath::Vector3 CameraPosition;
        DirectX::SimpleMath::Vector3 CameraDirection;

        bool UsingOrthographic = false;

        // TODO: Extract this into Params::DirectionalLight
        struct DirectionalLightParams
        {
            DirectX::SimpleMath::Color   DirectionalLightColour;
            DirectX::SimpleMath::Vector3 LightDirection;
            float LightIrradiance;
            DirectX::SimpleMath::Matrix ShadowTransform;
            GraphicsMemoryManager::DescriptorView ShadowMap;
        };

        DirectionalLightParams SunlightParams;
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);

        std::vector<Params::PointLight> PointLights;
        GraphicsMemoryManager::DescriptorView ShadowCubeArray;

    private:
        void InitializeRootSignature(ID3D12Device2* device);
        void InitializeShadowPSO(ID3D12Device2* device);
        void InitializePixelDepthReadPSO(ID3D12Device2* device);
        void InitializePixelDepthReadWritePSO(ID3D12Device2* device);
        void InitializeDepthWritePSO(ID3D12Device2* device);
        void ApplyDepthOnlyPipeline(ID3D12GraphicsCommandList* cl, bool multisampled, DrawType passType);

        RootSignature m_rootSignature;

        // All billboards are assumed to be masked
        std::unique_ptr<PipelineState> m_maskedShadowPSO;
        std::unique_ptr<PipelineState> m_maskedDepthWriteOnlyPSO;
        std::unique_ptr<PipelineState> m_maskedPixelDepthReadPSO;
        std::unique_ptr<PipelineState> m_maskedPixelDepthReadWritePSO;
    };
}