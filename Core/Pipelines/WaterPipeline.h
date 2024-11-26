#pragma once

#include "pch.h"
#include "Core/Pipelines/IRenderPipeline.h"
#include "Core/Pipelines/BufferStructs.h"
#include "Core/Rendering/DirectionalLight.h"
#include "Core/Parameters.h"
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
        static const size_t MAX_POINT_LIGHTS = 8;
        static constexpr int MAX_WAVES = 32;

        struct __declspec(align(16)) MatrixCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct __declspec(align(16)) LodCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float minLodDistance;
            DirectX::XMFLOAT3 cameraDirection;
            float maxLodDistance;
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

        struct __declspec(align(16)) PixelParamCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float maxAmplitude;
            DirectX::XMMATRIX shadowTransform;
            float thicknessPower;
            float sharpness;
            float refractiveIndex;
            float pad;
        };

        struct __declspec(align(16)) LightCB
        {
            AlignedDirectionalLight directionalLight;
            AlignedPointLight pointLights[MAX_POINT_LIGHTS];
            uint32_t numPointLights;
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
        void SetCameraDirection(DirectX::SimpleMath::Vector3 cameraDirection);
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);
        void SetPointLights(std::vector<Params::PointLight> pointLights);
        void SetEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        void SetTotalTime(float totalTimeSeconds);
        void SetWaterParams(Params::Water waterParams);

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
        DirectX::ConstantBuffer<MatrixCB> m_matrixCB;
        DirectX::ConstantBuffer<WaveCB> m_waveCB;
        DirectX::ConstantBuffer<LodCB> m_lodCB;
        DirectX::ConstantBuffer<PixelParamCB> m_pixelParamCB;
        DirectX::ConstantBuffer<LightCB> m_lightCB;

        std::shared_ptr<DirectX::CommonStates> m_states;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_comparisonSS;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        DirectX::SimpleMath::Vector3 m_cameraPosition;
        DirectX::SimpleMath::Vector3 m_cameraDirection;

        // TODO: Store a Params::DirectionalLight instead.
        DirectX::SimpleMath::Color   m_directionalLightColour;
        DirectX::SimpleMath::Vector3 m_lightDirection;
        float m_lightIrradiance;

        std::vector<Params::PointLight> m_pointLights;

        float m_totalTimeSeconds;
        float m_maxAmplitude = 0;

        std::array<Wave, 20> m_waves;
        Params::Water m_waterParams;
    };
}