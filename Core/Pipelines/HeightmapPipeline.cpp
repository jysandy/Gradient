#include "pch.h"
#include "Core/Pipelines/HeightmapPipeline.h"
#include "Core/ReadData.h"

namespace Gradient::Pipelines
{
    HeightmapPipeline::HeightmapPipeline(ID3D12Device2* device)
    {
        InitializeRootSignature(device);
        InitializeShadowPSO(device);
        InitializeRenderPSO(device);
        InitializeDepthWritePSO(device);
        InitializePixelDepthReadPSO(device);
    }

    void HeightmapPipeline::InitializeRootSignature(ID3D12Device* device)
    {
        m_rootSignature.AddCBV(0, 1);   // transforms
        m_rootSignature.AddCBV(1, 1);   // LOD settings
        m_rootSignature.AddCBV(2, 1);   // heightmap params
        m_rootSignature.AddCBV(0, 2);   // lights
        m_rootSignature.AddCBV(1, 2);   // camera and shadow transform
        m_rootSignature.AddSRV(0, 0);   // heightmap
        m_rootSignature.AddSRV(0, 2);   // albedo map
        m_rootSignature.AddSRV(1, 2);   // shadow map
        m_rootSignature.AddSRV(2, 2);   // normal map
        m_rootSignature.AddSRV(3, 2);   // ao map
        m_rootSignature.AddSRV(4, 2);   // metallic map
        m_rootSignature.AddSRV(5, 2);   // roughness map
        m_rootSignature.AddSRV(6, 2);   // environment map
        m_rootSignature.AddSRV(7, 2);   // point shadow maps
        m_rootSignature.AddSRV(8, 2);   // GTAO

        m_rootSignature.AddStaticSampler(
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR),
            0, 0
        );

        m_rootSignature.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(0),
            0, 2);

        m_rootSignature.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            0,
            1,
            D3D12_COMPARISON_FUNC_LESS_EQUAL),
            1, 2);

        m_rootSignature.Build(device);
    }

    void HeightmapPipeline::InitializeRenderPSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"Heightmap_VS.cso");
        auto hsData = DX::ReadData(L"Heightmap_HS.cso");
        auto dsData = DX::ReadData(L"Heightmap_DS.cso");
        auto psData = DX::ReadData(L"Heightmap_PS.cso");

        auto inputElements = std::array<D3D12_INPUT_ELEMENT_DESC, 3>();
        std::copy(VertexType::InputLayout.pInputElementDescs,
            VertexType::InputLayout.pInputElementDescs + 3,
            inputElements.begin());
        inputElements[0].SemanticName = "LOCALPOS";
        inputElements[0].SemanticIndex = 0;

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = { inputElements.data(), 3 };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.HS = { hsData.data(), hsData.size() };
        psoDesc.DS = { dsData.data(), dsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };
        //psoDesc.RasterizerState = DirectX::CommonStates::Wireframe;

        m_pso = std::make_unique<PipelineState>(psoDesc);
        m_pso->Build(device);
    }

    void HeightmapPipeline::InitializeShadowPSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultShadowDesc();

        auto vsData = DX::ReadData(L"Heightmap_VS.cso");
        auto hsData = DX::ReadData(L"Heightmap_HS.cso");
        auto dsData = DX::ReadData(L"Heightmap_DS.cso");

        auto inputElements = std::array<D3D12_INPUT_ELEMENT_DESC, 3>();
        std::copy(VertexType::InputLayout.pInputElementDescs,
            VertexType::InputLayout.pInputElementDescs + 3,
            inputElements.begin());
        inputElements[0].SemanticName = "LOCALPOS";
        inputElements[0].SemanticIndex = 0;

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = { inputElements.data(), 3 };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.HS = { hsData.data(), hsData.size() };
        psoDesc.DS = { dsData.data(), dsData.size() };

        // TODO: Look into biasing issues.
        // Probably replace the shadow pipeline with EVSM

        psoDesc.RasterizerState.DepthBias = 10000;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 10.f;

        m_shadowPso = std::make_unique<PipelineState>(psoDesc);
        m_shadowPso->Build(device);
    }

    void HeightmapPipeline::InitializeDepthWritePSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"Heightmap_VS.cso");
        auto hsData = DX::ReadData(L"Heightmap_HS.cso");
        auto dsData = DX::ReadData(L"Heightmap_DS.cso");

        auto inputElements = std::array<D3D12_INPUT_ELEMENT_DESC, 3>();
        std::copy(VertexType::InputLayout.pInputElementDescs,
            VertexType::InputLayout.pInputElementDescs + 3,
            inputElements.begin());
        inputElements[0].SemanticName = "LOCALPOS";
        inputElements[0].SemanticIndex = 0;

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = { inputElements.data(), 3 };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.HS = { hsData.data(), hsData.size() };
        psoDesc.DS = { dsData.data(), dsData.size() };

        m_depthWritePso = std::make_unique<PipelineState>(psoDesc);
        m_depthWritePso->Build(device);
    }

    void HeightmapPipeline::InitializePixelDepthReadPSO(ID3D12Device2* device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDepthWriteDisableDesc();

        auto vsData = DX::ReadData(L"Heightmap_VS.cso");
        auto hsData = DX::ReadData(L"Heightmap_HS.cso");
        auto dsData = DX::ReadData(L"Heightmap_DS.cso");
        auto psData = DX::ReadData(L"Heightmap_PS.cso");

        auto inputElements = std::array<D3D12_INPUT_ELEMENT_DESC, 3>();
        std::copy(VertexType::InputLayout.pInputElementDescs,
            VertexType::InputLayout.pInputElementDescs + 3,
            inputElements.begin());
        inputElements[0].SemanticName = "LOCALPOS";
        inputElements[0].SemanticIndex = 0;

        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.InputLayout = { inputElements.data(), 3 };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        psoDesc.VS = { vsData.data(), vsData.size() };
        psoDesc.HS = { hsData.data(), hsData.size() };
        psoDesc.DS = { dsData.data(), dsData.size() };
        psoDesc.PS = { psData.data(), psData.size() };
        //psoDesc.RasterizerState = DirectX::CommonStates::Wireframe;

        m_pixelDepthReadPso = std::make_unique<PipelineState>(psoDesc);
        m_pixelDepthReadPso->Build(device);
    }

    void HeightmapPipeline::ApplyShadowPipeline(ID3D12GraphicsCommandList* cl)
    {
        m_shadowPso->Set(cl, false);
        m_rootSignature.SetOnCommandList(cl);

        MatrixCB matrixConstants;
        matrixConstants.world = DirectX::XMMatrixTranspose(m_world);
        matrixConstants.view = DirectX::XMMatrixTranspose(m_view);
        matrixConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 1, matrixConstants);

        LodCB lodConstants;
        lodConstants.cameraDirection = m_cameraDirection;
        lodConstants.cameraPosition = m_cameraPosition;
        lodConstants.minLodDistance = 100;
        lodConstants.maxLodDistance = 400;
        lodConstants.cullingEnabled = 0; // Can't cull when drawing shadows!

        m_rootSignature.SetCBV(cl, 1, 1, lodConstants);

        HeightMapParamsCB heightConstants;
        heightConstants.height = m_heightMapComponent.Height;
        heightConstants.gridWidth = m_heightMapComponent.GridWidth;

        m_rootSignature.SetCBV(cl, 2, 1, heightConstants);

        m_rootSignature.SetSRV(cl, 0, 0, m_heightMapComponent.HeightMapTexture);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }

    void HeightmapPipeline::ApplyDepthWriteOnlyPipeline(ID3D12GraphicsCommandList* cl, bool multisampled)
    {
        m_depthWritePso->Set(cl, multisampled);
        m_rootSignature.SetOnCommandList(cl);

        MatrixCB matrixConstants;
        matrixConstants.world = DirectX::XMMatrixTranspose(m_world);
        matrixConstants.view = DirectX::XMMatrixTranspose(m_view);
        matrixConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 1, matrixConstants);

        LodCB lodConstants;
        lodConstants.cameraDirection = m_cameraDirection;
        lodConstants.cameraPosition = m_cameraPosition;
        lodConstants.minLodDistance = 100;
        lodConstants.maxLodDistance = 400;
        lodConstants.cullingEnabled = 1;

        m_rootSignature.SetCBV(cl, 1, 1, lodConstants);

        HeightMapParamsCB heightConstants;
        heightConstants.height = m_heightMapComponent.Height;
        heightConstants.gridWidth = m_heightMapComponent.GridWidth;

        m_rootSignature.SetCBV(cl, 2, 1, heightConstants);

        m_rootSignature.SetSRV(cl, 0, 0, m_heightMapComponent.HeightMapTexture);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }

    void HeightmapPipeline::Apply(ID3D12GraphicsCommandList* cl,
        bool multisampled,
        DrawType passType)
    {
        if (passType == DrawType::ShadowPass)
        {
            ApplyShadowPipeline(cl);
            return;
        }
        else if (passType == DrawType::DepthWriteOnly)
        {
            ApplyDepthWriteOnlyPipeline(cl, multisampled);
            return;
        }
        else if (passType == DrawType::PixelDepthReadOnly)
        {
            m_pixelDepthReadPso->Set(cl, multisampled);
        }
        else
        {
            m_pso->Set(cl, multisampled);
        }
        
        m_rootSignature.SetOnCommandList(cl);

        MatrixCB matrixConstants;
        matrixConstants.world = DirectX::XMMatrixTranspose(m_world);
        matrixConstants.view = DirectX::XMMatrixTranspose(m_view);
        matrixConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 1, matrixConstants);

        LodCB lodConstants;
        lodConstants.cameraDirection = m_cameraDirection;
        lodConstants.cameraPosition = m_cameraPosition;
        lodConstants.minLodDistance = 100;
        lodConstants.maxLodDistance = 400;
        lodConstants.cullingEnabled = 1;

        m_rootSignature.SetCBV(cl, 1, 1, lodConstants);

        HeightMapParamsCB heightConstants;
        heightConstants.height = m_heightMapComponent.Height;
        heightConstants.gridWidth = m_heightMapComponent.GridWidth;

        m_rootSignature.SetCBV(cl, 2, 1, heightConstants);

        LightCB lightConstants;

        lightConstants.directionalLight.colour = static_cast<DirectX::XMFLOAT3>(m_directionalLightColour);
        lightConstants.directionalLight.irradiance = m_lightIrradiance;
        lightConstants.directionalLight.direction = m_lightDirection;
        lightConstants.numPointLights = std::min(MAX_POINT_LIGHTS, m_pointLights.size());
        for (int i = 0; i < lightConstants.numPointLights; i++)
        {
            lightConstants.pointLights[i].colour = m_pointLights[i].Colour.ToVector3();
            lightConstants.pointLights[i].irradiance = m_pointLights[i].Irradiance;
            lightConstants.pointLights[i].position = m_pointLights[i].Position;
            lightConstants.pointLights[i].maxRange = m_pointLights[i].MaxRange;
            lightConstants.pointLights[i].shadowCubeIndex = m_pointLights[i].ShadowCubeIndex;

            auto viewMatrices = m_pointLights[i].GetViewMatrices();
            auto proj = m_pointLights[i].GetProjectionMatrix();
            for (int j = 0; j < 6; j++)
            {
                lightConstants.pointLights[i].shadowTransforms[j]
                    = DirectX::XMMatrixTranspose(viewMatrices[j]
                        * proj);
            }
        }

        m_rootSignature.SetCBV(cl, 0, 2, lightConstants);


        PixelParamCB pixelConstants;
        pixelConstants.cameraPosition = m_cameraPosition;
        pixelConstants.tiling = m_material.Tiling;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(m_shadowTransform);

        m_rootSignature.SetCBV(cl, 1, 2, pixelConstants);


        m_rootSignature.SetSRV(cl, 0, 0, 
            m_heightMapComponent.HeightMapTexture);
        m_rootSignature.SetSRV(cl, 0, 2, m_material.Texture);
        m_rootSignature.SetSRV(cl, 1, 2, m_shadowMap);
        m_rootSignature.SetSRV(cl, 2, 2, m_material.NormalMap);
        m_rootSignature.SetSRV(cl, 3, 2, m_material.AOMap);
        m_rootSignature.SetSRV(cl, 4, 2, m_material.MetalnessMap);
        m_rootSignature.SetSRV(cl, 5, 2, m_material.RoughnessMap);
        m_rootSignature.SetSRV(cl, 6, 2, m_environmentMap);
        m_rootSignature.SetSRV(cl, 7, 2, m_shadowCubeArray);
        m_rootSignature.SetSRV(cl, 8, 2, GTAOTexture);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }

    void HeightmapPipeline::SetHeightMapComponent(const ECS::Components::HeightMapComponent& hmComponent)
    {
        m_heightMapComponent = hmComponent;
    }

    void HeightmapPipeline::SetMaterial(const Rendering::PBRMaterial& material)
    {
        m_material = material;
    }

    void XM_CALLCONV HeightmapPipeline::SetWorld(DirectX::FXMMATRIX value)
    {
        m_world = value;
    }

    void XM_CALLCONV HeightmapPipeline::SetView(DirectX::FXMMATRIX value)
    {
        m_view = value;
    }

    void XM_CALLCONV HeightmapPipeline::SetProjection(DirectX::FXMMATRIX value)
    {
        m_proj = value;
    }

    void XM_CALLCONV HeightmapPipeline::SetMatrices(DirectX::FXMMATRIX world,
        DirectX::CXMMATRIX view,
        DirectX::CXMMATRIX projection)
    {
        SetWorld(world);
        SetView(view);
        SetProjection(projection);
    }

    void HeightmapPipeline::SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition)
    {
        m_cameraPosition = cameraPosition;
    }

    void HeightmapPipeline::SetCameraDirection(DirectX::SimpleMath::Vector3 cameraDirection)
    {
        m_cameraDirection = cameraDirection;
        m_cameraDirection.Normalize();
    }

    void HeightmapPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        m_directionalLightColour = dlight->GetColour();
        m_lightDirection = dlight->GetDirection();
        m_lightIrradiance = dlight->GetIrradiance();
        m_shadowMap = dlight->GetShadowMapSRV();
        m_shadowTransform = dlight->GetShadowTransform();
    }

    void HeightmapPipeline::SetPointLights(std::vector<Params::PointLight> pointLights)
    {
        m_pointLights = pointLights;
    }

    void HeightmapPipeline::SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_environmentMap = index;
    }

    void HeightmapPipeline::SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index)
    {
        m_shadowCubeArray = index;
    }
}