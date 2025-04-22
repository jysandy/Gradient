#pragma once

#include "pch.h"
#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/CommonStates.h>

#include "Core/Rendering/DirectionalLight.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/RootSignature.h"
#include "Core/PipelineState.h"

namespace Gradient::Pipelines
{
    class SkyDomePipeline : public IRenderPipeline
    {
    public:
        struct __declspec(align(256)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct __declspec(align(256)) PixelCB
        {
            DirectX::XMFLOAT3 sunColour;
            float sunExp;
            DirectX::XMFLOAT3 lightDirection;
            float sunCircleEnabled;
            float irradiance;
            float ambientIrradiance;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit SkyDomePipeline(ID3D12Device2* device);
        virtual ~SkyDomePipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl, 
            bool multisampled = true,
            DrawType passType = DrawType::PixelDepthReadWrite) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value);
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

        void SetDirectionalLight(Gradient::Rendering::DirectionalLight* dlight);
        void SetSunCircleEnabled(bool enabled);
        void SetAmbientIrradiance(float ambientIrradiance);

    private:
        RootSignature m_rootSignature;
        std::unique_ptr<PipelineState> m_pso;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;

        DirectX::SimpleMath::Vector3 m_cameraDirection;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        DirectX::SimpleMath::Color m_sunColour;
        float m_irradiance;
        float m_ambientIrradiance = 1.f;
        bool m_sunCircleEnabled;
    };
}