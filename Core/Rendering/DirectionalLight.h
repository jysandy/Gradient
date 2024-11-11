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

        DirectX::SimpleMath::Matrix GetShadowTransform() const;
        
        // Colour must be in linear space.
        void SetColour(DirectX::SimpleMath::Color colour);
        void SetLightDirection(const DirectX::SimpleMath::Vector3& direction);
        void SetIrradiance(float irradiance);
        void ClearAndSetDSV(ID3D11DeviceContext*);

        DirectX::SimpleMath::Color GetColour() const;
        float GetIrradiance() const;
        DirectX::SimpleMath::Vector3 GetDirection() const;

        DirectX::SimpleMath::Matrix GetView() const;
        DirectX::SimpleMath::Matrix GetProjection() const;

        ID3D11ShaderResourceView* GetShadowMapSRV() const;

    private:
        D3D11_VIEWPORT m_shadowMapViewport;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_shadowMapDS;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_shadowMapDSV;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowMapRSState;

        DirectX::SimpleMath::Color m_colour;
        float m_irradiance = 10.f;

        DirectX::SimpleMath::Vector3 m_sceneCentre;
        float m_sceneRadius;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        DirectX::SimpleMath::Matrix m_shadowMapView;
        DirectX::SimpleMath::Matrix m_shadowMapProj;

    };
}