#include "pch.h"

#include "Core/Pipelines/PBRPipeline.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/RootSignature.h"
#include <directxtk12/VertexTypes.h>
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    PBRPipeline::PBRPipeline(ID3D12Device* device)
    {
        m_rootSignature.AddCBV(0, 0);
        m_rootSignature.AddCBV(0, 1);
        m_rootSignature.AddCBV(1, 1);

        m_rootSignature.AddSRV(0, 1);
        m_rootSignature.AddSRV(1, 1);
        m_rootSignature.AddSRV(2, 1);
        m_rootSignature.AddSRV(3, 1);
        m_rootSignature.AddSRV(4, 1);
        m_rootSignature.AddSRV(5, 1);
        m_rootSignature.AddSRV(6, 1);
        m_rootSignature.AddSRV(7, 1);

        m_rootSignature.AddStaticSampler(
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC),
            0, 1);

        m_rootSignature.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0,
            1,
            D3D12_COMPARISON_FUNC_LESS),
            1,
            1);

        m_rootSignature.AddStaticSampler(
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR),
            3, 1);

        m_rootSignature.Build(device);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        auto vsData = DX::ReadData(L"WVP_VS.cso");
        auto psData = DX::ReadData(L"PBR_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = DirectX::CommonStates::DepthDefault;
        psoDesc.SampleMask = UINT_MAX;

        // TODO: Get render target state from the render texture
        psoDesc.RasterizerState = DirectX::CommonStates::CullCounterClockwise;
        psoDesc.NumRenderTargets = 1;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_PPV_ARGS(&m_singleSampledPSO)));

        psoDesc.RasterizerState.MultisampleEnable = TRUE;
        psoDesc.SampleDesc.Count = 4;
        psoDesc.SampleDesc.Quality = 0;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_PPV_ARGS(&m_multisampledPSO)));
    }

    void PBRPipeline::Apply(ID3D12GraphicsCommandList* cl, bool multisampled)
    {
        if (multisampled)
            cl->SetPipelineState(m_multisampledPSO.Get());
        else
            cl->SetPipelineState(m_singleSampledPSO.Get());
        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);

        auto lightBufferData = m_dLightCBData;
        lightBufferData.numPointLights = std::min(MAX_POINT_LIGHTS, m_pointLights.size());
        for (int i = 0; i < lightBufferData.numPointLights; i++)
        {
            lightBufferData.pointLights[i].colour = m_pointLights[i].Colour.ToVector3();
            lightBufferData.pointLights[i].irradiance = m_pointLights[i].Irradiance;
            lightBufferData.pointLights[i].position = m_pointLights[i].Position;
            lightBufferData.pointLights[i].maxRange = m_pointLights[i].MaxRange;
            lightBufferData.pointLights[i].shadowCubeIndex = m_pointLights[i].ShadowCubeIndex;

            auto viewMatrices = m_pointLights[i].GetViewMatrices();
            auto proj = m_pointLights[i].GetProjectionMatrix();
            for (int j = 0; j < 6; j++)
            {
                lightBufferData.pointLights[i].shadowTransforms[j]
                    = DirectX::XMMatrixTranspose(viewMatrices[j]
                        * proj);
            }
        }

        m_rootSignature.SetCBV(cl, 0, 1, lightBufferData);

        PixelCB pixelConstants;
        pixelConstants.cameraPosition = m_cameraPosition;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(m_shadowTransform);
        pixelConstants.emissiveRadiance = m_emissiveRadiance;

        m_rootSignature.SetCBV(cl, 1, 1, pixelConstants);

        m_rootSignature.SetSRV(cl, 0, 1, m_texture);
        m_rootSignature.SetSRV(cl, 1, 1, m_shadowMap);
        m_rootSignature.SetSRV(cl, 2, 1, m_normalMap);
        m_rootSignature.SetSRV(cl, 3, 1, m_aoMap);
        m_rootSignature.SetSRV(cl, 4, 1, m_metalnessMap);
        m_rootSignature.SetSRV(cl, 5, 1, m_roughnessMap);
        m_rootSignature.SetSRV(cl, 6, 1, m_environmentMap);
        m_rootSignature.SetSRV(cl, 7, 1, m_shadowCubeArray);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void PBRPipeline::SetAlbedo(GraphicsMemoryManager::DescriptorView index)
    {
        m_texture = index;
    }

    void PBRPipeline::SetNormalMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_normalMap = index;
    }

    void PBRPipeline::SetAOMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_aoMap = index;
    }

    void PBRPipeline::SetMetalnessMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_metalnessMap = index;
    }

    void PBRPipeline::SetRoughnessMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_roughnessMap = index;
    }

    void PBRPipeline::SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_environmentMap = index;
    }

    void PBRPipeline::SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index)
    {
        m_shadowCubeArray = index;
    }

    void PBRPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        m_shadowMap = dlight->GetShadowMapSRV();
        m_shadowTransform = dlight->GetShadowTransform();

        m_dLightCBData.directionalLight.colour = static_cast<DirectX::XMFLOAT3>(dlight->GetColour());
        m_dLightCBData.directionalLight.irradiance = dlight->GetIrradiance();
        m_dLightCBData.directionalLight.direction = dlight->GetDirection();
    }

    void PBRPipeline::SetPointLights(std::vector<Params::PointLight> pointLights)
    {
        m_pointLights = pointLights;
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

    void PBRPipeline::SetEmissiveRadiance(DirectX::SimpleMath::Vector3 emissiveRadiance)
    {
        m_emissiveRadiance = emissiveRadiance;
    }
}
