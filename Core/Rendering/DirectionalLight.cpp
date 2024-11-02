#include "pch.h"

#include "Core/Rendering/DirectionalLight.h"

namespace Gradient::Rendering
{
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    DirectionalLight::DirectionalLight(ID3D11Device* device,
        Vector3 lightDirection,
        float sceneRadius,
        Vector3 sceneCentre)
    {
        m_sceneRadius = sceneRadius;
        m_sceneCentre = sceneCentre;
        SetLightDirection(lightDirection);

        m_diffuse = Color(0.8f, 0.8f, 0.7f, 1.f);
        m_ambient = Color(0.1f, 0.1f, 0.1f, 1.f);
        m_specular = Color(0.8f, 0.8f, 0.4f, 1.f);

        m_shadowMapProj = SimpleMath::Matrix::CreateOrthographicOffCenter(
            -sceneRadius,
            sceneRadius,
            -sceneRadius,
            sceneRadius,
            sceneRadius,
            3 * sceneRadius
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

        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            DXGI_FORMAT_R32_TYPELESS,
            (int)shadowMapWidth,
            (int)shadowMapWidth,
            1, // Use a single array entry.
            1, // Use a single mipmap level.
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
        );

        DX::ThrowIfFailed(device->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_shadowMapDS.ReleaseAndGetAddressOf()
        ));

        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(
            D3D11_DSV_DIMENSION_TEXTURE2D,
            DXGI_FORMAT_D32_FLOAT
        );

        DX::ThrowIfFailed(device->CreateDepthStencilView(
            m_shadowMapDS.Get(),
            &dsvDesc,
            m_shadowMapDSV.ReleaseAndGetAddressOf()
        ));

        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_shadowMapDS.Get(),
            D3D11_SRV_DIMENSION_TEXTURE2D,
            DXGI_FORMAT_R32_FLOAT);

        DX::ThrowIfFailed(device->CreateShaderResourceView(
            m_shadowMapDS.Get(),
            &srvDesc,
            m_shadowMapSRV.ReleaseAndGetAddressOf()
        ));


    }

    void DirectionalLight::SetColours(Color ambient,
        Color diffuse,
        Color specular)
    {
        m_ambient = ambient;
        m_diffuse = diffuse;
        m_specular = specular;
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

    Color DirectionalLight::GetAmbient() const
    {
        return m_ambient;
    }

    Color DirectionalLight::GetDiffuse() const
    {
        return m_diffuse;
    }

    Color DirectionalLight::GetSpecular() const
    {
        return m_specular;
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

    void DirectionalLight::ClearAndSetDSV(ID3D11DeviceContext* context)
    {
        context->ClearDepthStencilView(m_shadowMapDSV.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.f, 0);

        ID3D11RenderTargetView* nullRTV = nullptr;

        context->OMSetRenderTargets(1, &nullRTV, m_shadowMapDSV.Get());
        context->RSSetViewports(1, &m_shadowMapViewport);
    }

    ID3D11ShaderResourceView* DirectionalLight::GetShadowMapSRV() const
    {
        return m_shadowMapSRV.Get();
    }
}