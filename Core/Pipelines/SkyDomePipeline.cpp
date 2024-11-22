#include "pch.h"

#include "Core/Pipelines/SkyDomePipeline.h"
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    SkyDomePipeline::SkyDomePipeline(ID3D11Device* device,
        std::shared_ptr<DirectX::CommonStates> states)
        : m_states(states)
    {
        m_vsData = DX::ReadData(L"skydome_vs.cso");

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

        auto psData = DX::ReadData(L"skydome_ps.cso");
        DX::ThrowIfFailed(
            device->CreatePixelShader(psData.data(),
                psData.size(),
                nullptr,
                m_ps.ReleaseAndGetAddressOf()));
        m_pixelCB.Create(device);
    }

    ID3D11InputLayout* SkyDomePipeline::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }

    void SkyDomePipeline::SetWorld(DirectX::FXMMATRIX _value)
    {
        // Unused
    }

    void SkyDomePipeline::SetView(DirectX::FXMMATRIX value)
    {
        DirectX::XMMATRIX view = value;
        // Ignore translation in the view matrix
        view.r[3] = DirectX::g_XMIdentityR3;
        m_view = view;
    }

    void SkyDomePipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void SkyDomePipeline::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
    {
        SetWorld(world);
        SetView(view);
        SetProjection(projection);
    }

    void SkyDomePipeline::Apply(ID3D11DeviceContext* context)
    {
        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_vertexCB.SetData(context, vertexConstants);

        context->VSSetShader(m_vs.Get(), nullptr, 0);
        auto cb = m_vertexCB.GetBuffer();
        context->VSSetConstantBuffers(0, 1, &cb);

        PixelCB pixelConstants;
        pixelConstants.sunColour = m_sunColour.ToVector3();
        pixelConstants.sunExp = 512;
        pixelConstants.lightDirection = m_lightDirection;
        pixelConstants.irradiance = m_irradiance;
        pixelConstants.ambientIrradiance = m_ambientIrradiance;
        if (m_sunCircleEnabled)
            pixelConstants.sunCircleEnabled = 1;
        else
            pixelConstants.sunCircleEnabled = 0;

        m_pixelCB.SetData(context, pixelConstants);
        cb = m_pixelCB.GetBuffer();
        context->PSSetConstantBuffers(0, 1, &cb);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
         
        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void SkyDomePipeline::SetDirectionalLight(Gradient::Rendering::DirectionalLight* dlight)
    {
        m_sunColour = dlight->GetColour();
        m_lightDirection = dlight->GetDirection();
        m_lightDirection.Normalize();
        m_irradiance = dlight->GetIrradiance();
    }

    void SkyDomePipeline::SetAmbientIrradiance(float ambientIrradiance)
    {
        m_ambientIrradiance = ambientIrradiance;
    }

    void SkyDomePipeline::SetSunCircleEnabled(bool enabled)
    {
        m_sunCircleEnabled = enabled;
    }
}