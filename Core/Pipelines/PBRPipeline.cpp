#include "pch.h"

#include "Core/Pipelines/PBRPipeline.h"
#include <directxtk/VertexTypes.h>
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    PBRPipeline::PBRPipeline(ID3D11Device* device,
        std::shared_ptr<DirectX::CommonStates> states)
        : m_states(states)
    {
        auto vsData = DX::ReadData(L"wvp_vs.cso");

        // TODO: Figure out how to support shader model 5.1+
        // Probably by setting the feature level
        DX::ThrowIfFailed(
            device->CreateVertexShader(vsData.data(),
                vsData.size(),
                nullptr,
                m_vs.ReleaseAndGetAddressOf()));

        auto psData = DX::ReadData(L"pbr_ps.cso");

        DX::ThrowIfFailed(
            device->CreatePixelShader(psData.data(),
                psData.size(),
                nullptr,
                m_ps.ReleaseAndGetAddressOf()));

        m_vertexCB.Create(device);
        m_pixelCameraCB.Create(device);
        m_dLightCB.Create(device);

        DX::ThrowIfFailed(
            device->CreateInputLayout(
                VertexType::InputElements,
                VertexType::InputElementCount,
                vsData.data(),
                vsData.size(),
                m_inputLayout.ReleaseAndGetAddressOf()
            ));

        CD3D11_SAMPLER_DESC cmpDesc(
            D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
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

    void PBRPipeline::Apply(ID3D11DeviceContext* context)
    {
        context->HSSetShader(nullptr, nullptr, 0);
        context->DSSetShader(nullptr, nullptr, 0);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_vertexCB.SetData(context, vertexConstants);

        m_dLightCB.SetData(context, m_dLightCBData);

        PixelCB pixelConstants;
        pixelConstants.cameraPosition = m_cameraPosition;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(m_shadowTransform);
        m_pixelCameraCB.SetData(context, pixelConstants);

        context->VSSetShader(m_vs.Get(), nullptr, 0);
        auto cb = m_vertexCB.GetBuffer();
        context->VSSetConstantBuffers(0, 1, &cb);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        cb = m_dLightCB.GetBuffer();
        context->PSSetConstantBuffers(0, 1, &cb);

        cb = m_pixelCameraCB.GetBuffer();
        context->PSSetConstantBuffers(1, 1, &cb);
        context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());

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
        context->RSSetState(m_states->CullCounterClockwise());

        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void PBRPipeline::SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_texture = srv;
    }

    void PBRPipeline::SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_normalMap = srv;
    }

    void PBRPipeline::SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_aoMap = srv;
    }

    void PBRPipeline::SetMetalnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_metalnessMap = srv;
    }

    void PBRPipeline::SetRoughnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_roughnessMap = srv;
    }

    void PBRPipeline::SetEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        m_environmentMap = srv;
    }

    void PBRPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        m_shadowMap = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>(dlight->GetShadowMapSRV());
        m_shadowTransform = dlight->GetShadowTransform();

        m_dLightCBData.colour = static_cast<DirectX::XMFLOAT3>(dlight->GetColour());
        m_dLightCBData.irradiance = dlight->GetIrradiance();
        m_dLightCBData.direction = dlight->GetDirection();
    }

    void PBRPipeline::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void PBRPipeline::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void PBRPipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void PBRPipeline::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
    {
        m_world = world;
        m_view = view;
        m_proj = projection;
    }

    void PBRPipeline::SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition)
    {
        m_cameraPosition = cameraPosition;
    }

    ID3D11InputLayout* PBRPipeline::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }
}