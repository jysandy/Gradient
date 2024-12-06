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
    class WaterShadowPipeline : public IRenderPipeline
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
            uint32_t cullingEnabled = 1;
            float pad[3];
        };

        struct __declspec(align(16)) WaveCB
        {
            uint32_t numWaves;
            float totalTimeSeconds;
            float pad[2];
            Wave waves[MAX_WAVES];
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit WaterShadowPipeline(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states);

        virtual void Apply(ID3D11DeviceContext* context) override;

        virtual ID3D11InputLayout* GetInputLayout() const override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
        void SetTotalTime(float totalTimeSeconds);
        void SetWaterParams(Params::Water waterParams);
        void SetWaves(const std::array<Wave, 20>& waves);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        Microsoft::WRL::ComPtr<ID3D11HullShader> m_hs;
        Microsoft::WRL::ComPtr<ID3D11DomainShader> m_ds;

        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowMapRSState;
        DirectX::ConstantBuffer<MatrixCB> m_matrixCB;
        DirectX::ConstantBuffer<WaveCB> m_waveCB;
        DirectX::ConstantBuffer<LodCB> m_lodCB;

        std::shared_ptr<DirectX::CommonStates> m_states;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;

        DirectX::SimpleMath::Vector3 m_cameraPosition;

        float m_totalTimeSeconds;
        float m_maxAmplitude = 0;

        std::array<Wave, 20> m_waves;
        Params::Water m_waterParams;
    };
}