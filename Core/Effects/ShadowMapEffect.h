#pragma once

#include "pch.h"
#include "Core/Effects/IEntityEffect.h"
#include "Core/Rendering/DirectionalLight.h"
#include <directxtk/Effects.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/BufferHelpers.h>

namespace Gradient::Effects
{
    class ShadowMapEffect : public IEntityEffect
    {
    public:
        struct __declspec(align(16)) VertexCB
        {
            DirectX::XMMATRIX world;
            DirectX::XMMATRIX view;
            DirectX::XMMATRIX proj;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit ShadowMapEffect(ID3D11Device* device);

        virtual void Apply(ID3D11DeviceContext* context) override;
        virtual void GetVertexShaderBytecode(
            void const** pShaderByteCode,
            size_t* pByteCodeLength) override;

        virtual void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) override;
        virtual ID3D11InputLayout* GetInputLayout() const override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetDirectionalLight(Rendering::DirectionalLight* dLight);

    private:
        std::vector<uint8_t> m_vsData;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowMapRSState;
        DirectX::ConstantBuffer<VertexCB> m_vertexCB;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
    };
}