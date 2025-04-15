#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/BufferStructs.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Parameters.h"
#include "Core/RootSignature.h"
#include "Core/PipelineState.h"
#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/CommonStates.h>
#include <array>

namespace Gradient::Pipelines
{
    class WaterPipeline : public IRenderPipeline
    {
    public:
        static const size_t MAX_POINT_LIGHTS = 8;
        static constexpr int MAX_WAVES = 32;

        struct __declspec(align(16)) MatrixCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct __declspec(align(16)) LodCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float minLodDistance;
            DirectX::XMFLOAT3 cameraDirection;
            float maxLodDistance;
            uint32_t cullingEnabled = 1;
            float pad[3];
        };

        struct __declspec(align(16)) WaveCB
        {
            uint32_t numWaves;
            float totalTimeSeconds;
            float pad[2];
            Wave waves[MAX_WAVES];
        };

        struct __declspec(align(16)) PixelParamCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float maxAmplitude;
            DirectX::XMMATRIX shadowTransform;
            float thicknessPower;
            float sharpness;
            float refractiveIndex;
            float pad;
        };

        struct __declspec(align(16)) LightCB
        {
            AlignedDirectionalLight directionalLight;
            AlignedPointLight pointLights[MAX_POINT_LIGHTS];
            uint32_t numPointLights;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit WaterPipeline(ID3D12Device* device);
        virtual ~WaterPipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl,
            bool multisampled = true,
            PassType passType = PassType::ForwardPass) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
        void SetCameraDirection(DirectX::SimpleMath::Vector3 cameraDirection);
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);
        void SetPointLights(std::vector<Params::PointLight> pointLights);
        void SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index);
        void SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index);
        void SetTotalTime(float totalTimeSeconds);
        void SetWaterParams(Params::Water waterParams);

        const std::array<Wave, 20>& GetWaves() const;

    private:
        void GenerateWaves();

        RootSignature m_rootSignature;
        std::unique_ptr<PipelineState> m_pso;

        GraphicsMemoryManager::DescriptorView m_shadowMap;
        GraphicsMemoryManager::DescriptorView m_environmentMap;
        GraphicsMemoryManager::DescriptorView m_shadowCubeArray;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        DirectX::SimpleMath::Vector3 m_cameraPosition;
        DirectX::SimpleMath::Vector3 m_cameraDirection;

        // TODO: Store a Params::DirectionalLight instead.
        DirectX::SimpleMath::Color   m_directionalLightColour;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        float m_lightIrradiance;

        std::vector<Params::PointLight> m_pointLights;

        float m_totalTimeSeconds;
        float m_maxAmplitude = 0;

        std::array<Wave, 20> m_waves;
        Params::Water m_waterParams;
    };
}