#pragma once

#include "pch.h"
#include "Core/Effects/IEntityEffect.h"
#include "Core/Rendering/DirectionalLight.h"
#include <directxtk/Effects.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/BufferHelpers.h>
#include <directxtk/CommonStates.h>

namespace Gradient::Effects
{
    class BlinnPhongEffect : public IEntityEffect
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
            DirectX::XMFLOAT3 cameraPosition;
            float pad;
            DirectX::XMMATRIX shadowTransform;
        };

        struct __declspec(align(16)) DLightCB
        {
            DirectX::XMFLOAT4 diffuse;
            DirectX::XMFLOAT4 ambient;
            DirectX::XMFLOAT4 specular;
            DirectX::XMFLOAT3 direction;
            float pad;
        };

        using VertexType = DirectX::VertexPositionNormalTexture;

        explicit BlinnPhongEffect(ID3D11Device* device, std::shared_ptr<DirectX::CommonStates> states);

        virtual void Apply(ID3D11DeviceContext* context) override;
        virtual void GetVertexShaderBytecode(
            void const** pShaderByteCode,
            size_t* pByteCodeLength) override;

        virtual void SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) override;
        virtual void SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) override;
        virtual void SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) override;
        virtual void SetMetalnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) override;
        virtual void SetRoughnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) override;
        virtual ID3D11InputLayout* GetInputLayout() const override;

        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        void SetCameraPosition(DirectX::SimpleMath::Vector3 cameraPosition);
        void SetDirectionalLight(Rendering::DirectionalLight* dlight);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_aoMap;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        DirectX::ConstantBuffer<VertexCB> m_vertexCB;
        DirectX::ConstantBuffer<PixelCB> m_pixelCameraCB;
        DirectX::ConstantBuffer<DLightCB> m_dLightCB;
        std::shared_ptr<DirectX::CommonStates> m_states;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_comparisonSS;
        
        std::vector<uint8_t> m_vsData;
        std::vector<uint8_t> m_psData;

        DirectX::SimpleMath::Matrix m_world;
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_shadowTransform;

        DirectX::SimpleMath::Vector3 m_cameraPosition;
        DLightCB m_dLightCBData;
    };
}