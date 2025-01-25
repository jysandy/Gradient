#include "pch.h"

#include "Core/Pipelines/SkyDomePipeline.h"
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    SkyDomePipeline::SkyDomePipeline(ID3D12Device* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        
        auto vsData = DX::ReadData(L"Skydome_VS.cso");
        auto psData = DX::ReadData(L"Skydome_PS.cso");

        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.RasterizerState = DirectX::CommonStates::CullCounterClockwise;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_PPV_ARGS(&m_pso)));

        m_rootSignature.AddCBV(0, 0);
        m_rootSignature.AddCBV(0, 1);

        m_rootSignature.Build(device);
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

    void SkyDomePipeline::Apply(ID3D12GraphicsCommandList* cl)
    {
        cl->SetPipelineState(m_pso.Get());
        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);

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

        m_rootSignature.SetCBV(cl, 0, 1, pixelConstants);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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