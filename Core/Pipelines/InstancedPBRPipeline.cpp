#include "pch.h"

#include "Core/Pipelines/InstancedPBRPipeline.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/RootSignature.h"
#include <directxtk12/VertexTypes.h>
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    InstancedPBRPipeline::InstancedPBRPipeline(ID3D12Device* device)
    {
        InitializeRootSignature(device);
        InitializeShadowPSO(device);
        InitializeRenderPSO(device);
    }

    void InstancedPBRPipeline::InitializeRootSignature(ID3D12Device* device)
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
    }

    void InstancedPBRPipeline::InitializeShadowPSO(ID3D12Device* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultShadowDesc();

        auto vsData = DX::ReadData(L"Instanced_VS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };
        // TODO: Set a PS to clip transparent pixels

        m_shadowPipelineState = std::make_unique<PipelineState>(psoDesc);
        m_shadowPipelineState->Build(device);
    }

    void InstancedPBRPipeline::InitializeRenderPSO(ID3D12Device* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"Instanced_VS.cso");
        auto psData = DX::ReadData(L"PBR_PS.cso");

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = VertexType::InputLayout;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };

        m_pipelineState = std::make_unique<PipelineState>(psoDesc);
        m_pipelineState->Build(device);
    }

    void InstancedPBRPipeline::ApplyShadowPipeline(ID3D12GraphicsCommandList* cl)
    {
        m_shadowPipelineState->Set(cl, false);
        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);
        m_rootSignature.SetStructuredBufferSRV(cl, 0, 0, m_instanceData);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void InstancedPBRPipeline::Apply(ID3D12GraphicsCommandList* cl,
        bool multisampled,
        bool drawingShadows)
    {
        if (drawingShadows)
        {
            ApplyShadowPipeline(cl);
            return;
        }

        m_pipelineState->Set(cl, multisampled);
        m_rootSignature.SetOnCommandList(cl);

        VertexCB vertexConstants;
        vertexConstants.world = DirectX::XMMatrixTranspose(m_world);
        vertexConstants.view = DirectX::XMMatrixTranspose(m_view);
        vertexConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, vertexConstants);
        m_rootSignature.SetStructuredBufferSRV(cl, 0, 0, m_instanceData);

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

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void InstancedPBRPipeline::SetMaterial(Rendering::PBRMaterial material)
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

    void InstancedPBRPipeline::SetInstanceData(const std::vector<ECS::Components::InstanceDataComponent::Instance>& instanceData)
    {
        m_instanceData.clear();

        for (const auto& instance : instanceData)
        {
            m_instanceData.push_back({
                DirectX::XMMatrixTranspose(instance.LocalTransform),
                instance.TexcoordURange,
                instance.TexcoordVRange
                });
        }
    }
}
