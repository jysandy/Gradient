#include "pch.h"

#include <array>
#include "Core/Pipelines/WaterPipeline.h"
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    WaterPipeline::WaterPipeline(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states)
    {
        m_states = states;

        auto vsData = DX::ReadData(L"passthrough_vs.cso");
        DX::ThrowIfFailed(
            device->CreateVertexShader(vsData.data(),
                vsData.size(),
                nullptr,
                m_vs.ReleaseAndGetAddressOf()));

        auto hsData = DX::ReadData(L"constant_hs.cso");
        DX::ThrowIfFailed(
          device->CreateHullShader(hsData.data(),
              hsData.size(),
              nullptr,
              m_hs.ReleaseAndGetAddressOf()));

        auto dsData = DX::ReadData(L"domain_shader_ds.cso");
        DX::ThrowIfFailed(
            device->CreateDomainShader(dsData.data(),
                dsData.size(),
                nullptr,
                m_ds.ReleaseAndGetAddressOf()));

        auto psData = DX::ReadData(L"Water_PS.cso");
        DX::ThrowIfFailed(
            device->CreatePixelShader(psData.data(),
                psData.size(),
                nullptr,
                m_ps.ReleaseAndGetAddressOf()));

        m_domainCB.Create(device);
        m_pixelCameraCB.Create(device);
        m_dLightCB.Create(device);

        auto inputElements = std::array<D3D11_INPUT_ELEMENT_DESC, VertexType::InputElementCount>();
        std::copy(VertexType::InputElements, VertexType::InputElements + 3, inputElements.begin());
        inputElements[0].SemanticName = "LOCALPOS";
        inputElements[0].SemanticIndex = 0;

        DX::ThrowIfFailed(
            device->CreateInputLayout(
                inputElements.data(),
                inputElements.size(),
                vsData.data(),
                vsData.size(),
                m_inputLayout.ReleaseAndGetAddressOf()
            ));

        CD3D11_SAMPLER_DESC cmpDesc(
            D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            0,
            1,
            D3D11_COMPARISON_LESS,
            nullptr,
            0,
            0
        );

        DX::ThrowIfFailed(
            device->CreateSamplerState(&cmpDesc,
                m_comparisonSS.ReleaseAndGetAddressOf()
            ));
    }

    void WaterPipeline::Apply(ID3D11DeviceContext* context)
    {
        context->VSSetShader(m_vs.Get(), nullptr, 0);
        context->HSSetShader(m_hs.Get(), nullptr, 0);

        context->DSSetShader(m_ds.Get(), nullptr, 0);
        DomainCB domainConstants;
        domainConstants.world = DirectX::XMMatrixTranspose(m_world);
        domainConstants.view = DirectX::XMMatrixTranspose(m_view);
        domainConstants.proj = DirectX::XMMatrixTranspose(m_proj);
        domainConstants.totalTimeSeconds = m_totalTimeSeconds;

        m_domainCB.SetData(context, domainConstants);
        auto cb = m_domainCB.GetBuffer();
        context->DSSetConstantBuffers(0, 1, &cb);

        context->PSSetShader(m_ps.Get(), nullptr, 0);
        PixelCB pixelConstants;
        pixelConstants.cameraPosition = m_cameraPosition;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(m_shadowTransform);
        m_pixelCameraCB.SetData(context, pixelConstants);
        
        DLightCB lightConstants;
        lightConstants.colour = static_cast<DirectX::XMFLOAT3>(m_directionalLightColour);
        lightConstants.irradiance = m_lightIrradiance;
        lightConstants.direction = m_lightDirection;
        m_dLightCB.SetData(context, lightConstants);
        
        cb = m_dLightCB.GetBuffer();
        context->PSSetConstantBuffers(0, 1, &cb);

        cb = m_pixelCameraCB.GetBuffer();
        context->PSSetConstantBuffers(1, 1, &cb);

        if (m_shadowMap != nullptr)
            context->PSSetShaderResources(1, 1, m_shadowMap.GetAddressOf());
        if (m_environmentMap != nullptr)
            context->PSSetShaderResources(2, 1, m_environmentMap.GetAddressOf());

        auto samplerState = m_states->LinearWrap();
        context->PSSetSamplers(0, 1, &samplerState);
        context->PSSetSamplers(1, 1, m_comparisonSS.GetAddressOf());

        // TODO: Change this once we're done with the water effect
        context->RSSetState(m_states->CullNone());

        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }

    ID3D11InputLayout* WaterPipeline::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }

    void XM_CALLCONV WaterPipeline::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void XM_CALLCONV WaterPipeline::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void XM_CALLCONV WaterPipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void XM_CALLCONV WaterPipeline::SetMatrices(DirectX::FXMMATRIX world,
        DirectX::CXMMATRIX view,
        DirectX::CXMMATRIX projection)
    {
        SetWorld(world);
        SetView(view);
        SetProjection(projection);
    }

    void WaterPipeline::SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition)
    {
        m_cameraPosition = cameraPosition;
    }

    void WaterPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight) 
    {
        m_directionalLightColour = dlight->GetColour();
        m_lightDirection = dlight->GetDirection();
        m_lightIrradiance = dlight->GetIrradiance();
        m_shadowMap = dlight->GetShadowMapSRV();
        m_shadowTransform = dlight->GetShadowTransform();
    }

    void WaterPipeline::SetEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_environmentMap = srv;
    }

    void WaterPipeline::SetTotalTime(float totalTimeSeconds)
    {
        m_totalTimeSeconds = totalTimeSeconds;
    }
}