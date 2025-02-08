#include "pch.h"

#include "Core/Pipelines/ShadowMapPipeline.h"
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    ShadowMapPipeline::ShadowMapPipeline(ID3D12Device* device)
    {
        m_rootSignature.AddCBV(0, 0);
        m_rootSignature.Build(device);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"WVP_VS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };

        auto rsDesc = CD3DX12_RASTERIZER_DESC1();
        rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rsDesc.CullMode = D3D12_CULL_MODE_BACK;
        rsDesc.FrontCounterClockwise = FALSE;
        rsDesc.DepthClipEnable = TRUE;
        rsDesc.MultisampleEnable = FALSE;
        rsDesc.AntialiasedLineEnable = FALSE;
        rsDesc.DepthBias = 10000;
        rsDesc.SlopeScaledDepthBias = 1.f;
        rsDesc.DepthBiasClamp = 0.f;

        psoDesc.RasterizerState = rsDesc;
        psoDesc.NumRenderTargets = 0;
        for (int i = 0; i < 8; i++)
            psoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

        m_pso = std::make_unique<PipelineState>(psoDesc);
        m_pso->Build(device);
    }
    
    void ShadowMapPipeline::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void ShadowMapPipeline::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void ShadowMapPipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void ShadowMapPipeline::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
    {
        m_world = world;
        m_view = view;
        m_proj = projection;
    }

    void ShadowMapPipeline::Apply(ID3D12GraphicsCommandList* cl, bool _multisampled)
    {
        m_pso->Set(cl, false);
        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
}