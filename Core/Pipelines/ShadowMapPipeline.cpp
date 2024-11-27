#include "pch.h"

#include "Core/Pipelines/ShadowMapPipeline.h"
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    ShadowMapPipeline::ShadowMapPipeline(ID3D11Device* device,
        std::shared_ptr<DirectX::CommonStates> states) 
        : m_states(states)
    {
        auto vsData = DX::ReadData(L"WVP_VS.cso");

        DX::ThrowIfFailed(
            device->CreateVertexShader(vsData.data(),
                vsData.size(),
                nullptr,
                m_vs.ReleaseAndGetAddressOf()));

        m_vertexCB.Create(device);

        device->CreateInputLayout(
            VertexType::InputElements,
            VertexType::InputElementCount,
            vsData.data(),
            vsData.size(),
            m_inputLayout.ReleaseAndGetAddressOf()
        );

        auto rsDesc = CD3D11_RASTERIZER_DESC();
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_BACK;
        rsDesc.FrontCounterClockwise = FALSE;
        rsDesc.DepthClipEnable = TRUE;
        rsDesc.ScissorEnable = FALSE;
        rsDesc.MultisampleEnable = FALSE;
        rsDesc.AntialiasedLineEnable = FALSE;
        rsDesc.DepthBias = 10000;
        rsDesc.SlopeScaledDepthBias = 1.f;
        rsDesc.DepthBiasClamp = 0.f;

        DX::ThrowIfFailed(device->CreateRasterizerState(&rsDesc,
            m_shadowMapRSState.ReleaseAndGetAddressOf()
        ));
    }
    
    ID3D11InputLayout* ShadowMapPipeline::GetInputLayout() const
    {
        return m_inputLayout.Get();
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

    void ShadowMapPipeline::Apply(ID3D11DeviceContext* context)
    {
        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_vertexCB.SetData(context, vertexConstants);

        context->VSSetShader(m_vs.Get(), nullptr, 0);
        auto cb = m_vertexCB.GetBuffer();
        context->VSSetConstantBuffers(0, 1, &cb);

        context->PSSetShader(nullptr, nullptr, 0);
        context->RSSetState(m_shadowMapRSState.Get());

        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
}