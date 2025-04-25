#include "pch.h"

#include "Core/Rendering/DirectionalLight.h"
#include <directxtk12/DirectXHelpers.h>

#include <array>

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

        const float shadowMapWidth = 4096.f;
        m_width = shadowMapWidth;

        m_shadowMapViewport = {
            0.0f,
            0.0f,
            shadowMapWidth,
            shadowMapWidth,
            0.f,
            1.f
        };

        auto depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS,
            (UINT64)shadowMapWidth,
            (UINT64)shadowMapWidth
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClearValue = {};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        m_shadowMapDS.Create(device,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue);

        auto gmm = GraphicsMemoryManager::Get();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        m_shadowMapDSV = gmm->CreateDSV(device, m_shadowMapDS.Get(), dsvDesc);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_shadowMapSRV = gmm->CreateSRV(device, m_shadowMapDS.Get(), &srvDesc);
    }

    void DirectionalLight::SetColour(DirectX::SimpleMath::Color colour)
    {
        m_colour = colour;
    }

    void DirectionalLight::SetLightDirection(const Vector3& lightDirection)
    {
        m_lightDirection = lightDirection;
        m_lightDirection.Normalize();

        auto lightPosition = m_sceneCentre - 2 * m_sceneRadius * m_lightDirection;

        m_shadowMapView = SimpleMath::Matrix::CreateLookAt(lightPosition,
            m_sceneCentre,
            SimpleMath::Vector3::UnitY);
        m_shadowMapViewInverse = m_shadowMapView.Invert();
    }

    void DirectionalLight::SetIrradiance(float irradiance)
    {
        m_irradiance = irradiance;
    }

    void DirectionalLight::SetCameraFrustum(
        const DirectX::BoundingFrustum& cameraFrustum)
    {
        DirectX::BoundingFrustum lightSpaceFrustum;
        cameraFrustum.Transform(lightSpaceFrustum, m_shadowMapView);

        std::array<DirectX::XMFLOAT3, lightSpaceFrustum.CORNER_COUNT> corners;
        lightSpaceFrustum.GetCorners(corners.data());

        DirectX::BoundingBox lightAABB;
        DirectX::BoundingBox::CreateFromPoints(lightAABB,
            corners.size(),
            corners.data(),
            sizeof(DirectX::XMFLOAT3));

        // Round the orthographic projection bounds to 
        // texel-size extents, as described in 
        // https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps#techniques-to-improve-shadow-maps
        // TODO: This doesn't seem to be working, figure out why
        /*float worldUnitsPerTexelX = lightAABB.Extents.x / (m_width + 1);
        float worldUnitsPerTexelY = lightAABB.Extents.y / (m_width + 1);

        auto vxBounds = DirectX::XMVectorSet(lightAABB.Center.x - lightAABB.Extents.x,
            lightAABB.Center.x + lightAABB.Extents.x, 0, 0);
        vxBounds /= worldUnitsPerTexelX;
        vxBounds = DirectX::XMVectorFloor(vxBounds);

        DirectX::XMFLOAT2 foo;
        DirectX::XMStoreFloat2(&foo, vxBounds);

        vxBounds *= worldUnitsPerTexelX;

        DirectX::XMFLOAT2 xBounds;
        DirectX::XMStoreFloat2(&xBounds, vxBounds);

        auto vyBounds = DirectX::XMVectorSet(lightAABB.Center.y - lightAABB.Extents.y,
            lightAABB.Center.y + lightAABB.Extents.y, 0, 0);
        vyBounds /= worldUnitsPerTexelY;
        vyBounds = DirectX::XMVectorFloor(vyBounds);
        vyBounds *= worldUnitsPerTexelY;

        DirectX::XMFLOAT2 yBounds;
        DirectX::XMStoreFloat2(&yBounds, vyBounds);

        m_shadowMapProj = SimpleMath::Matrix::CreateOrthographicOffCenter(
            xBounds.x,
            xBounds.y,
            yBounds.x,
            yBounds.y,
            m_sceneRadius,
            -lightAABB.Center.z + lightAABB.Extents.z
        );*/

        m_shadowMapProj = SimpleMath::Matrix::CreateOrthographicOffCenter(
            lightAABB.Center.x - lightAABB.Extents.x,
            lightAABB.Center.x + lightAABB.Extents.x,
            lightAABB.Center.y - lightAABB.Extents.y,
            lightAABB.Center.y + lightAABB.Extents.y,
            m_sceneRadius,
            -lightAABB.Center.z + lightAABB.Extents.z
        );

        {
            // Stretch the bounding box out to include the light's 
            // near plane. Add a bit of padding on the sides to prevent 
            // false positives.

            DirectX::BoundingBox temp;
            DirectX::BoundingBox::CreateFromPoints(temp,
                Vector3{ lightAABB.Center.x - lightAABB.Extents.x - 1, lightAABB.Center.y - lightAABB.Extents.y - 1, -m_sceneRadius },
                Vector3{ lightAABB.Center.x + lightAABB.Extents.x + 1, lightAABB.Center.y + lightAABB.Extents.y + 1, -m_sceneRadius + 1 });

            DirectX::BoundingBox::CreateMerged(lightAABB, lightAABB, temp);
        }

        DirectX::BoundingOrientedBox lightOBB;
        DirectX::BoundingOrientedBox::CreateFromBoundingBox(lightOBB,
            lightAABB);

        DirectX::BoundingOrientedBox worldSpaceLightOBB;
        lightOBB.Transform(worldSpaceLightOBB, m_shadowMapViewInverse);

        Vector3 lightSpacePosition{ lightAABB.Center.x,
            lightAABB.Center.y,
            lightAABB.Center.z + lightAABB.Extents.z };

        m_shadowBB = worldSpaceLightOBB;
        m_lightPosition = Vector3::Transform(lightSpacePosition, m_shadowMapViewInverse);
    }

    DirectX::SimpleMath::Vector3 DirectionalLight::GetPosition() const
    {
        return m_lightPosition;
    }

    DirectX::BoundingOrientedBox DirectionalLight::GetShadowBB() const
    {
        return m_shadowBB;
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

        auto cpuHandle = m_shadowMapDSV->GetCPUHandle();

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

    GraphicsMemoryManager::DescriptorView DirectionalLight::GetShadowMapSRV() const
    {
        return m_shadowMapSRV;
    }

    void DirectionalLight::TransitionToShaderResource(ID3D12GraphicsCommandList* cl)
    {
        m_shadowMapDS.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }
}