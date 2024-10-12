#pragma once

#include "pch.h"
#include <directxtk/Effects.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/BufferHelpers.h>
#include <directxtk/CommonStates.h>

namespace Gradient::Effects
{
    class BlinnPhongEffect : public DirectX::IEffect, public DirectX::IEffectMatrices
    {
    public:
        struct __declspec(align(16)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        struct __declspec(align(16)) PixelCameraCB
        {
            DirectX::XMFLOAT3 cameraPosition;
            float pad;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit BlinnPhongEffect(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states);

        virtual void Apply(ID3D11DeviceContext* context) override;
        virtual void GetVertexShaderBytecode(
            void const** pShaderByteCode,
            size_t* pByteCodeLength) override;

        void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);

        ID3D11InputLayout* GetInputLayout() const;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        DirectX::ConstantBuffer<VertexCB> m_vertexCB;
        DirectX::ConstantBuffer<PixelCameraCB> m_pixelCameraCB;
        std::shared_ptr<DirectX::CommonStates> m_states;

        std::vector<uint8_t> m_vsData;
        std::vector<uint8_t> m_psData;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;

        DirectX::SimpleMath::Vector3 m_cameraPosition;
    };
}