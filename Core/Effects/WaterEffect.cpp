#include "pch.h"

#include <array>
#include "Core/Effects/WaterEffect.h"
#include "Core/ReadData.h"

namespace Gradient::Effects
{
    WaterEffect::WaterEffect(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states)
    {
        m_states = states;

        m_vsData = DX::ReadData(L"passthrough_vs.cso");
        DX::ThrowIfFailed(
            device->CreateVertexShader(m_vsData.data(),
                m_vsData.size(),
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

        auto psData = DX::ReadData(L"pbr_ps.cso");
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
                m_vsData.data(),
                m_vsData.size(),
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

    void WaterEffect::Apply(ID3D11DeviceContext* context)
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
        lightConstants.diffuse = m_directionalLightColour;
        lightConstants.direction = m_lightDirection;
        m_dLightCB.SetData(context, lightConstants);
        
        cb = m_dLightCB.GetBuffer();
        context->PSSetConstantBuffers(0, 1, &cb);

        cb = m_pixelCameraCB.GetBuffer();
        context->PSSetConstantBuffers(1, 1, &cb);

        context->PSSetShaderResources(0, 1, m_albedo.GetAddressOf());

        if (m_shadowMap != nullptr)
            context->PSSetShaderResources(1, 1, m_shadowMap.GetAddressOf());
        if (m_normalMap != nullptr)
            context->PSSetShaderResources(2, 1, m_normalMap.GetAddressOf());
        if (m_aoMap != nullptr)
            context->PSSetShaderResources(3, 1, m_aoMap.GetAddressOf());
        if (m_metalnessMap != nullptr)
            context->PSSetShaderResources(4, 1, m_metalnessMap.GetAddressOf());
        if (m_roughnessMap != nullptr)
            context->PSSetShaderResources(5, 1, m_roughnessMap.GetAddressOf());
        if (m_environmentMap != nullptr)
            context->PSSetShaderResources(6, 1, m_environmentMap.GetAddressOf());

        auto samplerState = m_states->AnisotropicWrap();
        context->PSSetSamplers(0, 1, &samplerState);
        context->PSSetSamplers(1, 1, m_comparisonSS.GetAddressOf());
        samplerState = m_states->PointWrap();
        context->PSSetSamplers(2, 1, &samplerState);
        samplerState = m_states->LinearWrap();
        context->PSSetSamplers(3, 1, &samplerState);

        // TODO: Change this once we're done with the water effect
        context->RSSetState(m_states->CullNone());

        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }

    void WaterEffect::GetVertexShaderBytecode(
        void const** pShaderByteCode,
        size_t* pByteCodeLength)
    {
        assert(pShaderByteCode != nullptr && pByteCodeLength != nullptr);
        *pShaderByteCode = m_vsData.data();
        *pByteCodeLength = m_vsData.size();
    }

    void WaterEffect::SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_albedo = srv;
    }

    void WaterEffect::SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_normalMap = srv;
    }

    void WaterEffect::SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_aoMap = srv;
    }

    void WaterEffect::SetMetalnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_metalnessMap = srv;
    }

    void WaterEffect::SetRoughnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_roughnessMap = srv;
    }

    ID3D11InputLayout* WaterEffect::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }

    void XM_CALLCONV WaterEffect::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void XM_CALLCONV WaterEffect::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void XM_CALLCONV WaterEffect::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void XM_CALLCONV WaterEffect::SetMatrices(DirectX::FXMMATRIX world,
        DirectX::CXMMATRIX view,
        DirectX::CXMMATRIX projection)
    {
        SetWorld(world);
        SetView(view);
        SetProjection(projection);
    }

    void WaterEffect::SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition)
    {
        m_cameraPosition = cameraPosition;
    }

    void WaterEffect::SetDirectionalLight(Rendering::DirectionalLight* dlight) 
    {
        m_directionalLightColour = dlight->GetDiffuse();
        m_lightDirection = dlight->GetDirection();
    }

    void WaterEffect::SetEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_environmentMap = srv;
    }

    void WaterEffect::SetTotalTime(float totalTimeSeconds)
    {
        m_totalTimeSeconds = totalTimeSeconds;
    }
}