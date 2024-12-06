#include "pch.h"

#include <array>
#include "Core/Pipelines/WaterShadowPipeline.h"
#include "Core/ReadData.h"

#include <random>

namespace Gradient::Pipelines
{
    WaterShadowPipeline::WaterShadowPipeline(ID3D11Device* device, 
        std::shared_ptr<DirectX::CommonStates> states)
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

        m_matrixCB.Create(device);
        m_waveCB.Create(device);
        m_lodCB.Create(device);

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

    void WaterShadowPipeline::Apply(ID3D11DeviceContext* context)
    {
        MatrixCB matrixConstants;
        matrixConstants.world = DirectX::XMMatrixTranspose(m_world);
        matrixConstants.view = DirectX::XMMatrixTranspose(m_view);
        matrixConstants.proj = DirectX::XMMatrixTranspose(m_proj);
        m_matrixCB.SetData(context, matrixConstants);

        LodCB lodConstants;
        lodConstants.cameraPosition = m_cameraPosition;
        lodConstants.minLodDistance = m_waterParams.MinLod;
        lodConstants.maxLodDistance = m_waterParams.MaxLod;
        lodConstants.cullingEnabled = 0;
        m_lodCB.SetData(context, lodConstants);

        WaveCB waveConstants;
        for (int i = 0; i < m_waves.size(); i++)
        {
            waveConstants.waves[i] = m_waves[i];
        }
        waveConstants.numWaves = m_waves.size();
        waveConstants.totalTimeSeconds = m_totalTimeSeconds;
        m_waveCB.SetData(context, waveConstants);

        context->VSSetShader(m_vs.Get(), nullptr, 0);
        auto cb = m_waveCB.GetBuffer();
        context->VSSetConstantBuffers(0, 1, &cb);

        context->HSSetShader(m_hs.Get(), nullptr, 0);
        cb = m_matrixCB.GetBuffer();
        context->HSSetConstantBuffers(0, 1, &cb);
        cb = m_lodCB.GetBuffer();
        context->HSSetConstantBuffers(1, 1, &cb);

        context->DSSetShader(m_ds.Get(), nullptr, 0);
        cb = m_matrixCB.GetBuffer();
        context->DSSetConstantBuffers(0, 1, &cb);
        cb = m_waveCB.GetBuffer();
        context->DSSetConstantBuffers(1, 1, &cb);
        cb = m_lodCB.GetBuffer();
        context->DSSetConstantBuffers(2, 1, &cb);

        context->PSSetShader(nullptr, nullptr, 0);

        context->RSSetState(m_shadowMapRSState.Get());

        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }

    ID3D11InputLayout* WaterShadowPipeline::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }

    void XM_CALLCONV WaterShadowPipeline::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void XM_CALLCONV WaterShadowPipeline::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void XM_CALLCONV WaterShadowPipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void XM_CALLCONV WaterShadowPipeline::SetMatrices(DirectX::FXMMATRIX world,
        DirectX::CXMMATRIX view,
        DirectX::CXMMATRIX projection)
    {
        SetWorld(world);
        SetView(view);
        SetProjection(projection);
    }

    void WaterShadowPipeline::SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition)
    {
        m_cameraPosition = cameraPosition;
    }

    void WaterShadowPipeline::SetTotalTime(float totalTimeSeconds)
    {
        m_totalTimeSeconds = totalTimeSeconds;
    }

    void WaterShadowPipeline::SetWaterParams(Params::Water waterParams)
    {
        m_waterParams = waterParams;
    }

    void WaterShadowPipeline::SetWaves(const std::array<Wave, 20>& waves)
    {
        m_waves = waves;
    }
}