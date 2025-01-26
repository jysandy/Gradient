#include "pch.h"

#include "Core/Rendering/DirectionalLight.h"
#include <directxtk12/DirectXHelpers.h>

namespace Gradient::Rendering
{
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    DirectionalLight::DirectionalLight(ID3D12Device* device,
        Vector3 lightDirection,
        float sceneRadius,
        Vector3 sceneCentre)
    {
        m_sceneRadius = sceneRadius;
        m_sceneCentre = sceneCentre;
        SetLightDirection(lightDirection);

        m_colour = Color(0.8f, 0.8f, 0.7f, 1.f);
        m_irradiance = 10.f;

        m_shadowMapProj = SimpleMath::Matrix::CreateOrthographicOffCenter(
            -sceneRadius,
            sceneRadius,
            -sceneRadius,
            sceneRadius,
            sceneRadius,
            10 * sceneRadius
        );

        const float shadowMapWidth = 2048.f;

        m_shadowMapViewport = {
            0.0f,
            0.0f,
            shadowMapWidth,
            shadowMapWidth,
            0.f,
            1.f
        };

        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        depthStencilDesc.MipLevels = depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.Width = (int)shadowMapWidth;
        depthStencilDesc.Height = (int)shadowMapWidth;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClearValue = {};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &depthStencilDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthClearValue,
                IID_PPV_ARGS(m_shadowMapDS.ReleaseAndGetAddressOf())));
        m_shadowMapDS.SetState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

        auto gmm = GraphicsMemoryManager::Get();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        gmm->CreateDSV(device, m_shadowMapDS.Get(), dsvDesc);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        gmm->CreateSRV(device, m_shadowMapDS.Get(), &srvDesc);
    }

    void DirectionalLight::SetColour(DirectX::SimpleMath::Color colour)
    {
        m_colour = colour;
    }

    void DirectionalLight::SetLightDirection(const Vector3& lightDirection)
    {
        m_lightDirection = lightDirection;
        m_lightDirection.Normalize();

        auto lightPosition = -2 * m_sceneRadius * m_lightDirection;

        m_shadowMapView = SimpleMath::Matrix::CreateLookAt(lightPosition,
            m_sceneCentre,
            SimpleMath::Vector3::UnitY);
    }

    void DirectionalLight::SetIrradiance(float irradiance)
    {
        m_irradiance = irradiance;
    }

    Color DirectionalLight::GetColour() const
    {
        return m_colour;
    }

    float DirectionalLight::GetIrradiance() const
    {
        return m_irradiance;
    }

    Vector3 DirectionalLight::GetDirection() const
    {
        return m_lightDirection;
    }

    // Transforms a world space point into shadow map space.
    // X and Y become texcoords and Z becomes the depth.
    Matrix DirectionalLight::GetShadowTransform() const
    {
        const static auto t = DirectX::SimpleMath::Matrix(
            0.5f, 0.f, 0.f, 0.f,
            0.f, -0.5f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.5f, 0.5f, 0.f, 1.f
        );

        return m_shadowMapView * m_shadowMapProj * t;
    }

    Matrix DirectionalLight::GetView() const
    {
        return m_shadowMapView;
    }

    Matrix DirectionalLight::GetProjection() const
    {
        return m_shadowMapProj;
    }

    void DirectionalLight::ClearAndSetDSV(ID3D12GraphicsCommandList* cl)
    {
        auto gmm = GraphicsMemoryManager::Get();

        auto cpuHandle = gmm->GetDSVCpuHandle(m_shadowMapDSV);

        m_shadowMapDS.Transition(cl, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        cl->ClearDepthStencilView(
            cpuHandle,
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
            1.f, 
            0, 0, nullptr);

        cl->OMSetRenderTargets(
            0,
            nullptr,
            FALSE,
            &cpuHandle
        );

        cl->RSSetViewports(1, &m_shadowMapViewport);
    }

    GraphicsMemoryManager::DescriptorIndex DirectionalLight::GetShadowMapDescriptorIndex() const
    {
        return m_shadowMapSRV;
    }

    void DirectionalLight::TransitionToShaderResource(ID3D12GraphicsCommandList* cl)
    {
        m_shadowMapDS.Transition(cl, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}