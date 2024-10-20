#pragma once

#include "pch.h"

#include <directxtk/SimpleMath.h>

namespace Gradient::Rendering
{
    // A directional light that casts shadows.
    class DirectionalLight
    {
    public:
        DirectionalLight(ID3D11Device* device,
            DirectX::SimpleMath::Vector3 lightDirection,
            float sceneRadius,
            DirectX::SimpleMath::Vector3 sceneCentre = DirectX::SimpleMath::Vector3::Zero);

        DirectX::SimpleMath::Matrix GetShadowTransform();
        
        // Colours must be in linear space.
        void SetColours(DirectX::SimpleMath::Color ambient,
            DirectX::SimpleMath::Color diffuse,
            DirectX::SimpleMath::Color specular);
        
        void SetLightDirection(const DirectX::SimpleMath::Vector3& direction);
        void ClearAndSetDSV(ID3D11DeviceContext*);

        DirectX::SimpleMath::Color GetAmbient();
        DirectX::SimpleMath::Color GetDiffuse();
        DirectX::SimpleMath::Color GetSpecular();
        DirectX::SimpleMath::Vector3 GetDirection();

        DirectX::SimpleMath::Matrix GetView();
        DirectX::SimpleMath::Matrix GetProjection();

        ID3D11ShaderResourceView* GetShadowMapSRV();

    private:
        D3D11_VIEWPORT m_shadowMapViewport;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_shadowMapDS;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_shadowMapDSV;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowMapRSState;

        DirectX::SimpleMath::Color m_ambient;
        DirectX::SimpleMath::Color m_diffuse;
        DirectX::SimpleMath::Color m_specular;

        DirectX::SimpleMath::Vector3 m_sceneCentre;
        float m_sceneRadius;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        DirectX::SimpleMath::Matrix m_shadowMapView;
        DirectX::SimpleMath::Matrix m_shadowMapProj;

    };
}