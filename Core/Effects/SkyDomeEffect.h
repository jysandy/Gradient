#pragma once

#include "pch.h"
#include <directxtk/Effects.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/BufferHelpers.h>

#include "Core/Rendering/DirectionalLight.h"

namespace Gradient::Effects
{
    class SkyDomeEffect : public DirectX::IEffect, 
        public DirectX::IEffectMatrices
    {
    public:
        struct __declspec(align(16)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };    

        struct __declspec(align(16)) PixelCB
        {
            DirectX::XMFLOAT3 sunColour;
            float sunExp;
            DirectX::XMFLOAT3 lightDirection;
            float sunCircleEnabled;
            float irradiance;
            float ambientIrradiance;
        };

        using VertexType = DirectX::VertexPosition;

        explicit SkyDomeEffect(ID3D11Device* device);

        virtual void Apply(ID3D11DeviceContext* context) override;
        virtual void GetVertexShaderBytecode(
            void const** pShaderByteCode,
            size_t* pByteCodeLength) override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        ID3D11InputLayout* GetInputLayout() const;
        void SetDirectionalLight(Gradient::Rendering::DirectionalLight* dlight);
        void SetSunCircleEnabled(bool enabled);
        void SetAmbientIrradiance(float ambientIrradiance);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        DirectX::ConstantBuffer<VertexCB> m_vertexCB;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
        DirectX::ConstantBuffer<PixelCB> m_pixelCB;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;

        DirectX::SimpleMath::Vector3 m_cameraDirection;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        DirectX::SimpleMath::Color m_sunColour;
        float m_irradiance;
        float m_ambientIrradiance = 1.f;
        bool m_sunCircleEnabled;

        std::vector<uint8_t> m_vsData;
    };
}