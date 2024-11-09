#include "pch.h"

#include "Core/Effects/ShadowMapEffect.h"
#include "Core/ReadData.h"

namespace Gradient::Effects
{
    ShadowMapEffect::ShadowMapEffect(ID3D11Device* device)
    {
        m_vsData = DX::ReadData(L"wvp_vs.cso");

        DX::ThrowIfFailed(
            device->CreateVertexShader(m_vsData.data(),
                m_vsData.size(),
                nullptr,
                m_vs.ReleaseAndGetAddressOf()));

        m_vertexCB.Create(device);

        device->CreateInputLayout(
            VertexType::InputElements,
            VertexType::InputElementCount,
            m_vsData.data(),
            m_vsData.size(),
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

    void ShadowMapEffect::GetVertexShaderBytecode(
        void const** pShaderByteCode,
        size_t* pByteCodeLength)
    {
        assert(pShaderByteCode != nullptr && pByteCodeLength != nullptr);
        *pShaderByteCode = m_vsData.data();
        *pByteCodeLength = m_vsData.size();
    }
    
    ID3D11InputLayout* ShadowMapEffect::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }

    void ShadowMapEffect::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void ShadowMapEffect::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void ShadowMapEffect::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void ShadowMapEffect::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
    {
        m_world = world;
        m_view = view;
        m_proj = projection;
    }

    void ShadowMapEffect::Apply(ID3D11DeviceContext* context)
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
    }

    void ShadowMapEffect::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        SetView(dlight->GetView());
        SetProjection(dlight->GetProjection());
    }
}