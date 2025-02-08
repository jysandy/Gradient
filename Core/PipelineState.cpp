#include "pch.h"
#include "Core/PipelineState.h"

#include <directxtk12/VertexTypes.h>
#include <directxtk12/CommonStates.h>

namespace Gradient
{
    PipelineState::PipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc)
        : m_desc(psoDesc)
    {
    }

    void PipelineState::Build(ID3D12Device* device)
    {
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&m_desc,
                IID_PPV_ARGS(&m_singleSampledPSO)));

        auto multisampledPSODesc = m_desc;

        multisampledPSODesc.RasterizerState.MultisampleEnable = TRUE;
        multisampledPSODesc.SampleDesc.Count = 4;
        multisampledPSODesc.SampleDesc.Quality = 0;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&multisampledPSODesc,
                IID_PPV_ARGS(&m_multisampledPSO)));
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineState::GetDefaultDesc()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        psoDesc.InputLayout = DirectX::VertexPositionNormalTexture::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = DirectX::CommonStates::DepthDefault;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState = DirectX::CommonStates::CullCounterClockwise;
        psoDesc.NumRenderTargets = 1;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;

        return psoDesc;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineState::GetDesc()
    {
        return m_desc;
    }

    void PipelineState::Set(ID3D12GraphicsCommandList* cl,
        bool multisampled)
    {
        if (multisampled)
            cl->SetPipelineState(m_multisampledPSO.Get());
        else
            cl->SetPipelineState(m_singleSampledPSO.Get());
    }
}