#include "pch.h"

#include "Core/Pipelines/InstancedPBRPipeline.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/RootSignature.h"
#include <directxtk12/VertexTypes.h>
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    InstancedPBRPipeline::InstancedPBRPipeline(ID3D12Device2* device)
    {
        InitializeRootSignature(device);
        InitializeShadowPSO(device);
        InitializeDepthWritePSO(device);
        InitializePixelDepthReadPSO(device);
        InitializePixelDepthReadWritePSO(device);
    }

    void InstancedPBRPipeline::InitializeRootSignature(ID3D12Device2* device)
    {
        m_rootSignature.AddCBV(0, 0);
        m_rootSignature.AddCBV(0, 1);
        m_rootSignature.AddCBV(1, 1);

        m_rootSignature.AddRootSRV(0, 0); // instance data
        m_rootSignature.AddSRV(0, 1);
        m_rootSignature.AddSRV(1, 1);
        m_rootSignature.AddSRV(2, 1);
        m_rootSignature.AddSRV(3, 1);
        m_rootSignature.AddSRV(4, 1);
        m_rootSignature.AddSRV(5, 1);
        m_rootSignature.AddSRV(6, 1);
        m_rootSignature.AddSRV(7, 1);
        m_rootSignature.AddSRV(8, 1);

        m_rootSignature.AddStaticSampler(
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC),
            0, 1);

        m_rootSignature.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            0,
            1,
            D3D12_COMPARISON_FUNC_LESS_EQUAL),
            1,
            1);

        m_rootSignature.AddStaticSampler(
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR),
            3, 1);

        m_rootSignature.Build(device);
    }

    void InstancedPBRPipeline::InitializeShadowPSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultShadowDesc();

        auto vsData = DX::ReadData(L"Instanced_VS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };

        m_unmaskedShadowPipelineState = std::make_unique<PipelineState>(psoDesc);
        m_unmaskedShadowPipelineState->Build(device);

        auto maskedPSData = DX::ReadData(L"MaskedDepth_PS.cso");

        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedShadowPipelineState = std::make_unique<PipelineState>(psoDesc);
        m_maskedShadowPipelineState->Build(device);
    }

    void InstancedPBRPipeline::InitializePixelDepthReadPSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDepthWriteDisableDesc();

        auto vsData = DX::ReadData(L"Instanced_VS.cso");
        auto psData = DX::ReadData(L"PBR_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };

        m_unmaskedPixelDepthReadPSO = std::make_unique<PipelineState>(psoDesc);
        m_unmaskedPixelDepthReadPSO->Build(device);

        auto maskedPSData = DX::ReadData(L"PBR_Masked_PS.cso");

        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedPixelDepthReadPSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedPixelDepthReadPSO->Build(device);
    }

    void InstancedPBRPipeline::InitializePixelDepthReadWritePSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"Instanced_VS.cso");
        auto psData = DX::ReadData(L"PBR_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };

        m_unmaskedPixelDepthReadWritePSO = std::make_unique<PipelineState>(psoDesc);
        m_unmaskedPixelDepthReadWritePSO->Build(device);

        auto maskedPSData = DX::ReadData(L"PBR_Masked_PS.cso");

        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedPixelDepthReadWritePSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedPixelDepthReadWritePSO->Build(device);
    }

    void InstancedPBRPipeline::InitializeDepthWritePSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"Instanced_VS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };

        m_unmaskedDepthWriteOnlyPSO = std::make_unique<PipelineState>(psoDesc);
        m_unmaskedDepthWriteOnlyPSO->Build(device);

        auto maskedPSData = DX::ReadData(L"MaskedDepth_PS.cso");

        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedDepthWriteOnlyPSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedDepthWriteOnlyPSO->Build(device);
    }

    void InstancedPBRPipeline::ApplyDepthOnlyPipeline(ID3D12GraphicsCommandList* cl,
        bool multisampled,
        DrawType passType)
    {
        if (passType == DrawType::ShadowPass)
        {
            if (m_material.Masked)
            {
                m_maskedShadowPipelineState->Set(cl, false);
            }
            else
            {
                m_unmaskedShadowPipelineState->Set(cl, false);
            }
        }
        else if (passType == DrawType::DepthWriteOnly)
        {
            if (m_material.Masked)
            {
                m_maskedDepthWriteOnlyPSO->Set(cl, multisampled);
            }
            else
            {
                m_unmaskedDepthWriteOnlyPSO->Set(cl, multisampled);
            }
        }

        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.viewProj = DirectX::XMMatrixTranspose(m_view * m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);
        m_rootSignature.SetStructuredBufferSRV(cl, 0, 0, m_instanceHandle);
        m_rootSignature.SetSRV(cl, 0, 1, m_material.Texture);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void InstancedPBRPipeline::Apply(ID3D12GraphicsCommandList* cl,
        bool multisampled,
        DrawType passType)
    {
        if (passType == DrawType::ShadowPass || passType == DrawType::DepthWriteOnly)
        {
            ApplyDepthOnlyPipeline(cl, multisampled, passType);
            return;
        }
        else if (passType == DrawType::PixelDepthReadOnly)
        {
            if (m_material.Masked)
            {
                m_maskedPixelDepthReadPSO->Set(cl, multisampled);
            }
            else
            {
                m_unmaskedPixelDepthReadPSO->Set(cl, multisampled);
            }
        }
        else if (passType == DrawType::PixelDepthReadWrite)
        {
            if (m_material.Masked)
            {
                m_maskedPixelDepthReadWritePSO->Set(cl, multisampled);
            }
            else
            {
                m_unmaskedPixelDepthReadWritePSO->Set(cl, multisampled);
            }
        }


        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.viewProj = DirectX::XMMatrixTranspose(m_view * m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);
        m_rootSignature.SetStructuredBufferSRV(cl, 0, 0, m_instanceHandle);

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
        pixelConstants.tiling = m_material.Tiling;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(m_shadowTransform);
        pixelConstants.emissiveRadiance = m_material.EmissiveRadiance;

        m_rootSignature.SetCBV(cl, 1, 1, pixelConstants);

        m_rootSignature.SetSRV(cl, 0, 1, m_material.Texture);
        m_rootSignature.SetSRV(cl, 1, 1, m_shadowMap);
        m_rootSignature.SetSRV(cl, 2, 1, m_material.NormalMap);
        m_rootSignature.SetSRV(cl, 3, 1, m_material.AOMap);
        m_rootSignature.SetSRV(cl, 4, 1, m_material.MetalnessMap);
        m_rootSignature.SetSRV(cl, 5, 1, m_material.RoughnessMap);
        m_rootSignature.SetSRV(cl, 6, 1, m_environmentMap);
        m_rootSignature.SetSRV(cl, 7, 1, m_shadowCubeArray);
        m_rootSignature.SetSRV(cl, 8, 1, GTAOTexture);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void InstancedPBRPipeline::SetMaterial(const Rendering::PBRMaterial& material)
    {
        m_material = material;
    }

    void InstancedPBRPipeline::SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_environmentMap = index;
    }

    void InstancedPBRPipeline::SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index)
    {
        m_shadowCubeArray = index;
    }

    void InstancedPBRPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        m_shadowMap = dlight->GetShadowMapSRV();
        m_shadowTransform = dlight->GetShadowTransform();

        m_dLightCBData.directionalLight.colour = static_cast<DirectX::XMFLOAT3>(dlight->GetColour());
        m_dLightCBData.directionalLight.irradiance = dlight->GetIrradiance();
        m_dLightCBData.directionalLight.direction = dlight->GetDirection();
    }

    void InstancedPBRPipeline::SetPointLights(std::vector<Params::PointLight> pointLights)
    {
        m_pointLights = pointLights;
    }

    void InstancedPBRPipeline::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void InstancedPBRPipeline::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void InstancedPBRPipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void InstancedPBRPipeline::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
    {
        m_world = world;
        m_view = view;
        m_proj = projection;
    }

    void InstancedPBRPipeline::SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition)
    {
        m_cameraPosition = cameraPosition;
    }

    void InstancedPBRPipeline::SetInstanceData(const ECS::Components::InstanceDataComponent& instanceComponent)
    {
        m_instanceHandle = instanceComponent.BufferHandle;
    }
}
