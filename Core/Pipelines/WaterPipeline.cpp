#include "pch.h"

#include <array>
#include "Core/Pipelines/WaterPipeline.h"
#include "Core/ReadData.h"

#include <random>

namespace Gradient::Pipelines
{
    WaterPipeline::WaterPipeline(ID3D12Device2* device)
    {
        m_rootSignature.AddCBV(0, 0);
        m_rootSignature.AddCBV(0, 1);
        m_rootSignature.AddCBV(1, 1);
        m_rootSignature.AddCBV(0, 2);
        m_rootSignature.AddCBV(1, 2);
        m_rootSignature.AddCBV(2, 2);
        m_rootSignature.AddCBV(0, 3);
        m_rootSignature.AddCBV(1, 3);

        m_rootSignature.AddSRV(1, 3);
        m_rootSignature.AddSRV(2, 3);
        m_rootSignature.AddSRV(3, 3);

        m_rootSignature.AddStaticSampler(
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR),
            0, 3);

        m_rootSignature.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            0,
            1,
            D3D12_COMPARISON_FUNC_LESS_EQUAL),
            1, 3);

        m_rootSignature.Build(device);


        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = PipelineState::GetDefaultDesc();

        auto vsData = DX::ReadData(L"Water_VS.cso");
        auto hsData = DX::ReadData(L"Water_HS.cso");
        auto dsData = DX::ReadData(L"Water_DS.cso");
        auto psData = DX::ReadData(L"Water_PS.cso");

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

        m_pso = std::make_unique<PipelineState>(psoDesc);
        m_pso->Build(device);

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
                std::round((m_waves.size() - (float)i) * 16.f / m_waves.size()));

            m_maxAmplitude += m_waves[i].amplitude;

            amplitudeFactor *= 0.9;
        }
    }

    void WaterPipeline::Apply(ID3D12GraphicsCommandList* cl,
        bool multisampled,
        DrawType passType)
    {
        if (passType != DrawType::PixelDepthReadWrite) return;

        m_pso->Set(cl, multisampled);
        m_rootSignature.SetOnCommandList(cl);

        MatrixCB matrixConstants;
        matrixConstants.world = DirectX::XMMatrixTranspose(m_world);
        matrixConstants.view = DirectX::XMMatrixTranspose(m_view);
        matrixConstants.proj = DirectX::XMMatrixTranspose(m_proj);

        m_rootSignature.SetCBV(cl, 0, 0, matrixConstants);
        m_rootSignature.SetCBV(cl, 0, 1, matrixConstants);
        m_rootSignature.SetCBV(cl, 0, 2, matrixConstants);

        LodCB lodConstants;
        lodConstants.cameraDirection = m_cameraDirection;
        lodConstants.cameraPosition = m_cameraPosition;
        lodConstants.minLodDistance = m_waterParams.MinLod;
        lodConstants.maxLodDistance = m_waterParams.MaxLod;
        lodConstants.cullingEnabled = 1;

        m_rootSignature.SetCBV(cl, 1, 1, lodConstants);
        m_rootSignature.SetCBV(cl, 2, 2, lodConstants);

        WaveCB waveConstants;
        for (int i = 0; i < m_waves.size(); i++)
        {
            waveConstants.waves[i] = m_waves[i];
        }
        waveConstants.numWaves = m_waves.size();
        waveConstants.totalTimeSeconds = m_totalTimeSeconds;

        m_rootSignature.SetCBV(cl, 0, 0, waveConstants);
        m_rootSignature.SetCBV(cl, 1, 2, waveConstants);

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

        m_rootSignature.SetCBV(cl, 0, 3, lightConstants);

        PixelParamCB pixelConstants;
        pixelConstants.cameraPosition = m_cameraPosition;
        pixelConstants.maxAmplitude = m_maxAmplitude;
        pixelConstants.shadowTransform = DirectX::XMMatrixTranspose(m_shadowTransform);
        pixelConstants.thicknessPower = m_waterParams.Scattering.ThicknessPower;
        pixelConstants.sharpness = m_waterParams.Scattering.Sharpness;
        pixelConstants.refractiveIndex = m_waterParams.Scattering.RefractiveIndex;

        m_rootSignature.SetCBV(cl, 1, 3, pixelConstants);

        m_rootSignature.SetSRV(cl, 1, 3, m_shadowMap);
        m_rootSignature.SetSRV(cl, 2, 3, m_environmentMap);
        m_rootSignature.SetSRV(cl, 3, 3, m_shadowCubeArray);

        cl->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
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

    void WaterPipeline::SetCameraDirection(DirectX::SimpleMath::Vector3 cameraDirection)
    {
        m_cameraDirection = cameraDirection;
        m_cameraDirection.Normalize();
    }

    void WaterPipeline::SetDirectionalLight(Rendering::DirectionalLight* dlight)
    {
        m_directionalLightColour = dlight->GetColour();
        m_lightDirection = dlight->GetDirection();
        m_lightIrradiance = dlight->GetIrradiance();
        m_shadowMap = dlight->GetShadowMapSRV();
        m_shadowTransform = dlight->GetShadowTransform();
    }

    void WaterPipeline::SetPointLights(std::vector<Params::PointLight> pointLights)
    {
        m_pointLights = pointLights;
    }

    void WaterPipeline::SetEnvironmentMap(GraphicsMemoryManager::DescriptorView index)
    {
        m_environmentMap = index;
    }

    void WaterPipeline::SetShadowCubeArray(GraphicsMemoryManager::DescriptorView index)
    {
        m_shadowCubeArray = index;
    }

    void WaterPipeline::SetTotalTime(float totalTimeSeconds)
    {
        m_totalTimeSeconds = totalTimeSeconds;
    }

    void WaterPipeline::SetWaterParams(Params::Water waterParams)
    {
        m_waterParams = waterParams;
    }

    const std::array<Wave, 20>& WaterPipeline::GetWaves() const
    {
        return m_waves;
    }
}