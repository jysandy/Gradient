#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Rendering/DirectionalLight.h"
#include <directxtk/Effects.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/BufferHelpers.h>
#include <directxtk/CommonStates.h>
#include <array>

namespace Gradient::Pipelines
{
    class WaterPipeline : public IRenderPipeline
    {
    public:
        static constexpr int MAX_WAVES = 32;

        struct __declspec(align(16)) HullCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMFLOAT3 cameraPosition;
            float pad;
        };

        struct __declspec(align(16)) DomainCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct Wave
        {
            float amplitude;
            float wavelength;
            float speed;
            float sharpness;
            DirectX::XMFLOAT3 direction;
            float pad;
        };

        struct __declspec(align(16)) WaveCB
        {
            uint32_t numWaves;
            float totalTimeSeconds;
            float pad[2];
            Wave waves[MAX_WAVES];
        };

        struct __declspec(align(16)) PixelCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float maxAmplitude;
            DirectX::XMMATRIX shadowTransform;
        };

        struct __declspec(align(16)) DLightCB
        {
            DirectX::XMFLOAT3 colour;
            float irradiance;
            DirectX::XMFLOAT3 direction;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit WaterPipeline(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states);

        virtual void Apply(ID3D11DeviceContext* context) override;

        virtual ID3D11InputLayout* GetInputLayout() const override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);
        void SetEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        void SetTotalTime(float totalTimeSeconds);

    private:
        void GenerateWaves();

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        Microsoft::WRL::ComPtr<ID3D11HullShader> m_hs;
        Microsoft::WRL::ComPtr<ID3D11DomainShader> m_ds;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_albedo;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_aoMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_metalnessMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_roughnessMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_environmentMap;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        DirectX::ConstantBuffer<HullCB> m_hullCB;
        DirectX::ConstantBuffer<DomainCB> m_domainCB;
        DirectX::ConstantBuffer<WaveCB> m_waveCB;
        DirectX::ConstantBuffer<PixelCB> m_pixelCameraCB;
        DirectX::ConstantBuffer<DLightCB> m_dLightCB;

        std::shared_ptr<DirectX::CommonStates> m_states;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_comparisonSS;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        DirectX::SimpleMath::Vector3 m_cameraPosition;

        DirectX::SimpleMath::Color   m_directionalLightColour;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        float m_lightIrradiance;

        float m_totalTimeSeconds;
        float m_maxAmplitude = 0;

        std::array<Wave, 16> m_waves;
    };
}