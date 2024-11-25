#include "pch.h"

#include <array>
#include "Core/Pipelines/WaterPipeline.h"
#include "Core/ReadData.h"

#include <random>

namespace Gradient::Pipelines
{
    WaterPipeline::WaterPipeline(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states)
    {
        m_states = states;

        auto vsData = DX::ReadData(L"Water_VS.cso");
        DX::ThrowIfFailed(
            device->CreateVertexShader(vsData.data(),
                vsData.size(),
                nullptr,
                m_vs.ReleaseAndGetAddressOf()));

        auto hsData = DX::ReadData(L"Water_HS.cso");
        DX::ThrowIfFailed(
            device->CreateHullShader(hsData.data(),
                hsData.size(),
                nullptr,
                m_hs.ReleaseAndGetAddressOf()));

        auto dsData = DX::ReadData(L"Water_DS.cso");
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

        m_hullCB.Create(device);
        m_domainCB.Create(device);
        m_pixelCameraCB.Create(device);
        m_dLightCB.Create(device);
        m_waveCB.Create(device);

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

        GenerateWaves();
    }

    void WaterPipeline::GenerateWaves()
    {
        float maxAmplitude = 0.1f;
        float maxWavelength = 5.f;
        float maxSpeed = 5.f;
        DirectX::SimpleMath::Vector3 direction{ -1, 0, 0.8 };
        direction.Normalize();

        std::random_device rd{};
        std::mt19937 gen{ rd() };

        std::normal_distribution xRng{ direction.x, 0.2f };
        std::normal_distribution zRng{ direction.z, 0.2f };

        float amplitudeFactor = 0.7f;
        for (int i = 0; i < m_waves.size(); i++)
        {
            auto d = DirectX::SimpleMath::Vector3{
                xRng(gen),
                0,
                zRng(gen)
            };
            d.Normalize();

            m_waves[i].direction = d;
            m_waves[i].amplitude = maxAmplitude * amplitudeFactor;
            m_waves[i].wavelength = std::max(0.2f, maxWavelength * amplitudeFactor);
            m_waves[i].speed = (1 - amplitudeFactor) * maxSpeed;
            m_waves[i].sharpness = std::max(1.f, 
                std::round((m_waves.size() - (float)i) * 10.f / m_waves.size()));
           
            m_maxAmplitude += m_waves[i].amplitude;

            amplitudeFactor *= 0.9;
        }
    }

    void WaterPipeline::Apply(ID3D11DeviceContext* context)
    {
        WaveCB waveConstants;
        for (int i = 0; i < m_waves.size(); i++)
        {
            waveConstants.waves[i] = m_waves[i];
        }
        waveConstants.numWaves = m_waves.size();
        waveConstants.totalTimeSeconds = m_totalTimeSeconds;
        m_waveCB.SetData(context, waveConstants);
        auto cb = m_waveCB.GetBuffer();
        context->VSSetConstantBuffers(0, 1, &cb);
        context->VSSetShader(m_vs.Get(), nullptr, 0);
        
        context->HSSetShader(m_hs.Get(), nullptr, 0);

        HullCB hullConstants;
        hullConstants.world = DirectX::XMMatrixTranspose(m_world);
        hullConstants.cameraPosition = m_cameraPosition;
        m_hullCB.SetData(context, hullConstants);
        cb = m_hullCB.GetBuffer();
        context->HSSetConstantBuffers(0, 1, &cb);

        context->DSSetShader(m_ds.Get(), nullptr, 0);

        DomainCB domainConstants;
        domainConstants.world = DirectX::XMMatrixTranspose(m_world);
        domainConstants.view = DirectX::XMMatrixTranspose(m_view);
        domainConstants.proj = DirectX::XMMatrixTranspose(m_proj);
        m_domainCB.SetData(context, domainConstants);
        cb = m_domainCB.GetBuffer();
        context->DSSetConstantBuffers(0, 1, &cb);

        cb = m_waveCB.GetBuffer();
        context->DSSetConstantBuffers(1, 1, &cb);

        context->PSSetShader(m_ps.Get(), nullptr, 0);
        PixelCB pixelConstants;
        pixelConstants.cameraPosition = m_cameraPosition;
        pixelConstants.maxAmplitude = m_maxAmplitude;
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

        context->RSSetState(m_states->CullCounterClockwise());
        //context->RSSetState(m_states->Wireframe());

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