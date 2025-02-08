#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/RootSignature.h"
#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/CommonStates.h>

namespace Gradient::Pipelines
{
    class ShadowMapPipeline : public IRenderPipeline
    {
    public:
        struct __declspec(align(16)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit ShadowMapPipeline(ID3D12Device* device);
        virtual ~ShadowMapPipeline() noexcept = default;

        virtual void Apply(ID3D12GraphicsCommandList* cl, bool multisampled = false) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

    private:
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
        RootSignature m_rootSignature;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
    };
}