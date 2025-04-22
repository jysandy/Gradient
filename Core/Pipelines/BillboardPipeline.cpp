#include "pch.h"

#include "Core/Pipelines/BillboardPipeline.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/RootSignature.h"
#include <directxtk12/VertexTypes.h>
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    BillboardPipeline::BillboardPipeline(ID3D12Device2* device)
    {
        InitializeRootSignature(device);
        InitializeShadowPSO(device);
        InitializeDepthWritePSO(device);
        InitializePixelDepthReadPSO(device);
        InitializePixelDepthReadWritePSO(device);
    }

    void BillboardPipeline::InitializeRootSignature(ID3D12Device2* device)
    {
        m_rootSignature.AddCBV(0, 0);
        m_rootSignature.AddCBV(1, 0);
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

    void BillboardPipeline::InitializeShadowPSO(ID3D12Device2* device)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultShadowMeshDesc();

        auto msData = DX::ReadData(L"Billboard_MS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.MS = { msData.data(), msData.size() };

        auto maskedPSData = DX::ReadData(L"MaskedDepth_PS.cso");

        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedShadowPSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedShadowPSO->Build(device);
    }

    void BillboardPipeline::InitializePixelDepthReadPSO(ID3D12Device2* device)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDepthWriteDisableMeshDesc();

        auto msData = DX::ReadData(L"Billboard_MS.cso");
        auto maskedPSData = DX::ReadData(L"PBR_Masked_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.MS = { msData.data(), msData.size() };
        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedPixelDepthReadPSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedPixelDepthReadPSO->Build(device);
    }

    void BillboardPipeline::InitializePixelDepthReadWritePSO(ID3D12Device2* device)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultMeshDesc();

        auto msData = DX::ReadData(L"Billboard_MS.cso");
        auto maskedPSData = DX::ReadData(L"PBR_Masked_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.MS = { msData.data(), msData.size() };
        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };

        m_maskedPixelDepthReadWritePSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedPixelDepthReadWritePSO->Build(device);
    }

    void BillboardPipeline::InitializeDepthWritePSO(ID3D12Device2* device)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultMeshDesc();

        auto msData = DX::ReadData(L"Billboard_MS.cso");
        auto maskedPSData = DX::ReadData(L"MaskedDepth_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.MS = { msData.data(), msData.size() };
        psoDesc.PS = { maskedPSData.data(), maskedPSData.size() };
        m_maskedDepthWriteOnlyPSO = std::make_unique<PipelineState>(psoDesc);
        m_maskedDepthWriteOnlyPSO->Build(device);
    }

    void BillboardPipeline::ApplyDepthOnlyPipeline(ID3D12GraphicsCommandList* cl,
        bool multisampled,
        DrawType passType)
    {
        if (passType == DrawType::ShadowPass)
        {
            m_maskedShadowPSO->Set(cl, false);
        }
        else if (passType == DrawType::DepthWriteOnly)
        {
            m_maskedDepthWriteOnlyPSO->Set(cl, multisampled);
        }

        m_rootSignature.SetOnCommandList(cl);

        MatrixCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(World);
        vertexConstants.viewProj = DirectX::XMMatrixTranspose(View * Proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);
        m_rootSignature.SetStructuredBufferSRV(cl, 0, 0, InstanceHandle);
        m_rootSignature.SetSRV(cl, 0, 1, Material.Texture);

        DrawParamsCB drawConstants;
        drawConstants.cameraPosition = CameraPosition;
        drawConstants.cameraDirection = CameraDirection;
        drawConstants.cardWidth = CardDimensions.x;
        drawConstants.cardHeight = CardDimensions.y;
        drawConstants.numInstances = InstanceCount;
        if (UsingOrthographic)
        {
            drawConstants.useCameraDirectionForCulling = 1;
        }
        else
        {
            drawConstants.useCameraDirectionForCulling = 0;
        }
        for (int i = 0; i < 6; i++)
        {
            drawConstants.cullingFrustumPlanes[i] = CullingFrustumPlanes[i];
        }
        m_rootSignature.SetCBV(cl, 1, 0, drawConstants);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void BillboardPipeline::Apply(ID3D12GraphicsCommandList* cl,
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
                m_maskedPixelDepthReadPSO->Set(cl, multisampled);
        }
        else if (passType == DrawType::PixelDepthReadWrite)
        {
                m_maskedPixelDepthReadWritePSO->Set(cl, multisampled);
        }


        m_rootSignature.SetOnCommandList(cl);

        MatrixCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(World);
        vertexConstants.viewProj = DirectX::XMMatrixTranspose(View * Proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);
        m_rootSignature.SetStructuredBufferSRV(cl, 0, 0, InstanceHandle);

        DrawParamsCB drawConstants;
        drawConstants.cameraPosition = CameraPosition;
        drawConstants.cameraDirection = CameraDirection;
        drawConstants.cardWidth = CardDimensions.x;
        drawConstants.cardHeight = CardDimensions.y;
        drawConstants.numInstances = InstanceCount;
        if (UsingOrthographic)
        {
            drawConstants.useCameraDirectionForCulling = 1;
        }
        else
        {
            drawConstants.useCameraDirectionForCulling = 0;
        }
        for (int i = 0; i < 6; i++)
        {
            drawConstants.cullingFrustumPlanes[i] = CullingFrustumPlanes[i];
        }
        m_rootSignature.SetCBV(cl, 1, 0, drawConstants);

        LightCB lightBufferData;
        lightBufferData.directionalLight.colour = SunlightParams.DirectionalLightColour.ToVector3();
        lightBufferData.directionalLight.direction = SunlightParams.LightDirection;
        lightBufferData.directionalLight.irradiance = SunlightParams.LightIrradiance;

        lightBufferData.numPointLights = std::min(MAX_POINT_LIGHTS, PointLights.size());
        for (int i = 0; i < lightBufferData.numPointLights; i++)
        {
            lightBufferData.pointLights[i].colour = PointLights[i].Colour.ToVector3();
            lightBufferData.pointLights[i].irradiance = PointLights[i].Irradiance;
            lightBufferData.pointLights[i].position = PointLights[i].Position;
            lightBufferData.pointLights[i].maxRange = PointLights[i].MaxRange;
            lightBufferData.pointLights[i].shadowCubeIndex = PointLights[i].ShadowCubeIndex;

            auto viewMatrices = PointLights[i].GetViewMatrices();
            auto proj = PointLights[i].GetProjectionMatrix();
            for (int j = 0; j < 6; j++)
            {
                lightBufferData.pointLights[i].shadowTransforms[j]
                    = DirectX::XMMatrixTranspose(viewMatrices[j]
                        * proj);
            }
        }

        m_rootSignature.SetCBV(cl, 0, 1, lightBufferData);

        PixelCB pixelConstants;
        pixelConstants.cameraPosition = CameraPosition;
        pixelConstants.tiling = Material.Tiling;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(SunlightParams.ShadowTransform);
        pixelConstants.emissiveRadiance = Material.EmissiveRadiance;

        m_rootSignature.SetCBV(cl, 1, 1, pixelConstants);

        m_rootSignature.SetSRV(cl, 0, 1, Material.Texture);
        m_rootSignature.SetSRV(cl, 1, 1, SunlightParams.ShadowMap);
        m_rootSignature.SetSRV(cl, 2, 1, Material.NormalMap);
        m_rootSignature.SetSRV(cl, 3, 1, Material.AOMap);
        m_rootSignature.SetSRV(cl, 4, 1, Material.MetalnessMap);
        m_rootSignature.SetSRV(cl, 5, 1, Material.RoughnessMap);
        m_rootSignature.SetSRV(cl, 6, 1, EnvironmentMap);
        m_rootSignature.SetSRV(cl, 7, 1, ShadowCubeArray);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void BillboardPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        SunlightParams.ShadowMap = dlight->GetShadowMapSRV();
        SunlightParams.ShadowTransform = dlight->GetShadowTransform();

        SunlightParams.DirectionalLightColour = dlight->GetColour();
        SunlightParams.LightIrradiance = dlight->GetIrradiance();
        SunlightParams.LightDirection = dlight->GetDirection();
    }
}
