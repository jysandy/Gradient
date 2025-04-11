#pragma once

#include "pch.h"
#include "Core/BarrierResource.h"
#include "Core/GraphicsMemoryManager.h"
#include <directxtk12/SimpleMath.h>

namespace Gradient::Rendering
{
    // A directional light that casts shadows.
    class DirectionalLight
    {
    public:
        DirectionalLight(ID3D12Device* device,
            DirectX::SimpleMath::Vector3 lightDirection,
            float sceneRadius,
            DirectX::SimpleMath::Vector3 sceneCentre = DirectX::SimpleMath::Vector3::Zero);

        DirectX::SimpleMath::Matrix GetShadowTransform() const;
        
        // Colour must be in linear space.
        void SetColour(DirectX::SimpleMath::Color colour);
        void SetLightDirection(const DirectX::SimpleMath::Vector3& direction);
        void SetIrradiance(float irradiance);
        // WIP
        void SetCameraFrustum( 
            const DirectX::BoundingFrustum& cameraFrustum);

        void ClearAndSetDSV(ID3D12GraphicsCommandList* cl);
        void TransitionToShaderResource(ID3D12GraphicsCommandList* cl);

        DirectX::SimpleMath::Color GetColour() const;
        float GetIrradiance() const;
        DirectX::SimpleMath::Vector3 GetDirection() const;

        DirectX::SimpleMath::Matrix GetView() const;
        DirectX::SimpleMath::Matrix GetProjection() const;

        GraphicsMemoryManager::DescriptorView GetShadowMapSRV() const;

    private:
        D3D12_VIEWPORT m_shadowMapViewport;
        GraphicsMemoryManager::DescriptorView m_shadowMapSRV;
        BarrierResource m_shadowMapDS;
        GraphicsMemoryManager::DescriptorView m_shadowMapDSV;

        DirectX::SimpleMath::Color m_colour;
        float m_irradiance = 10.f;

        DirectX::SimpleMath::Vector3 m_sceneCentre;
        float m_sceneRadius;
        float m_width;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        DirectX::SimpleMath::Matrix m_shadowMapView;
        DirectX::SimpleMath::Matrix m_shadowMapViewInverse;
        DirectX::SimpleMath::Matrix m_shadowMapProj;

    };
}