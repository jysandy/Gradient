#include "pch.h"

#include "Core/Effects/BlinnPhongEffect.h"
#include <directxtk/VertexTypes.h>
#include "Core/ReadData.h"

namespace Gradient::Effects
{
    BlinnPhongEffect::BlinnPhongEffect(ID3D11Device* device,
        std::shared_ptr<DirectX::CommonStates> states)
    {
        m_states = states;
        m_vsData = DX::ReadData(L"blinn_phong_vs.cso");

        // TODO: Figure out how to support shader model 5.1+
        // Probably by setting the feature level
        DX::ThrowIfFailed(
            device->CreateVertexShader(m_vsData.data(),
                m_vsData.size(),
                nullptr,
                m_vs.ReleaseAndGetAddressOf()));

        m_psData = DX::ReadData(L"blinn_phong_ps.cso");

        DX::ThrowIfFailed(
            device->CreatePixelShader(m_psData.data(),
                m_psData.size(),
                nullptr,
                m_ps.ReleaseAndGetAddressOf()));

        m_vertexCB.Create(device);

        device->CreateInputLayout(
            VertexType::InputElements,
            VertexType::InputElementCount,
            m_vsData.data(),
            m_vsData.size(),
            m_inputLayout.ReleaseAndGetAddressOf()
        );
    }

    void BlinnPhongEffect::GetVertexShaderBytecode(
        void const** pShaderByteCode,
        size_t* pByteCodeLength)
    {
        assert(pShaderByteCode != nullptr && pByteCodeLength != nullptr);
        *pShaderByteCode = m_vsData.data();
        *pByteCodeLength = m_vsData.size();
    }

    void BlinnPhongEffect::Apply(ID3D11DeviceContext* context)
    {
        context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_vertexCB.SetData(context, vertexConstants);

        context->VSSetShader(m_vs.Get(), nullptr, 0);
        auto cb = m_vertexCB.GetBuffer();
        context->VSSetConstantBuffers(0, 1, &cb);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
        auto samplerState = m_states->LinearWrap();
        context->PSSetSamplers(0, 1, &samplerState);
    }

    void BlinnPhongEffect::SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_texture = srv;
    }

    void BlinnPhongEffect::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void BlinnPhongEffect::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void BlinnPhongEffect::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void BlinnPhongEffect::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
    {
        m_world = world;
        m_view = view;
        m_proj = projection;
    }

    ID3D11InputLayout* BlinnPhongEffect::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }
}
